import argparse
import copy
import json
import os
import re
import time
from datetime import datetime, timezone
from typing import Any, Callable


DEEPSEEK_API_URL = "https://api.deepseek.com/chat/completions"
DEFAULT_CONFIG_PATH = "experiment_config.json"
LOG_DIR = os.environ.get("INTERNOVEL_LOG_DIR", "logs")
LOCAL_ENV_PATH = ".env"
PROMPT_VERSION = "skill-lab-v1"
EXPECTED_LABELS = ["A", "B", "X", "Y"]
STATE_KEYS = ["characters", "locations", "inventory", "open_threads", "facts"]

DEFAULT_CONFIG: dict[str, Any] = {
    "world": "中世纪猎巫题材，融合魔法与科技元素",
    "style_ratio": {"magic": 10, "sci-fi": 90, "mystery": 0, "romance": 0},
    "model": "deepseek-v4-flash",
    "api_key": "",
    "temperature": 0.7,
    "max_tokens": 900,
    "request_timeout": 30,
    "pipeline": "single_step",
    "context": {"recent_turns": 2, "use_state": False},
    "judge": {"enabled": False, "model": None},
}

EMPTY_STATE = {
    "characters": [],
    "locations": [],
    "inventory": [],
    "open_threads": [],
    "facts": [],
}


def utc_now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


def new_session_log_path() -> str:
    os.makedirs(LOG_DIR, exist_ok=True)
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    return os.path.join(LOG_DIR, f"novel_session_{stamp}.jsonl")


class ConversationSessionLog:
    """JSONL session log: one structured event per line."""

    def __init__(self, path: str):
        self.path = path
        self._fp = open(path, "w", encoding="utf-8")

    def write_event(self, event: dict[str, Any]) -> None:
        self._fp.write(json.dumps(event, ensure_ascii=False) + "\n")
        self._fp.flush()

    def close(self) -> None:
        self._fp.close()


def deep_merge(base: dict[str, Any], override: dict[str, Any]) -> dict[str, Any]:
    result = copy.deepcopy(base)
    for key, value in override.items():
        if isinstance(value, dict) and isinstance(result.get(key), dict):
            result[key] = deep_merge(result[key], value)
        else:
            result[key] = value
    return result


def load_config(path: str | None) -> dict[str, Any]:
    config = copy.deepcopy(DEFAULT_CONFIG)
    if path and os.path.exists(path):
        with open(path, "r", encoding="utf-8") as fp:
            config = deep_merge(config, json.load(fp))
    validate_config(config)
    return config


def validate_config(config: dict[str, Any]) -> None:
    if config.get("pipeline") not in {"three_step", "single_step"}:
        raise ValueError("pipeline 必须是 three_step 或 single_step")
    if not isinstance(config.get("world"), str) or not config["world"].strip():
        raise ValueError("world 不能为空")
    if not isinstance(config.get("style_ratio"), dict):
        raise ValueError("style_ratio 必须是对象")
    if not isinstance(config.get("context"), dict):
        raise ValueError("context 必须是对象")
    if not isinstance(config.get("judge"), dict):
        raise ValueError("judge 必须是对象")


def config_for_log(config: dict[str, Any]) -> dict[str, Any]:
    redacted = copy.deepcopy(config)
    if redacted.get("api_key"):
        redacted["api_key"] = "***"
    return redacted


def api_key_from_local_env(path: str = LOCAL_ENV_PATH) -> str:
    if not os.path.exists(path):
        return ""
    with open(path, "r", encoding="utf-8") as fp:
        for line in fp:
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            name, value = line.split("=", 1)
            if name.strip() == "DEEPSEEK_API_KEY":
                return value.strip().strip("\"'")
    return ""


def api_key(config: dict[str, Any] | None = None) -> str:
    key = os.environ.get("DEEPSEEK_API_KEY", "").strip()
    if not key and config:
        key = str(config.get("api_key", "")).strip()
    if not key:
        key = api_key_from_local_env()
    if not key:
        raise RuntimeError("请设置 DEEPSEEK_API_KEY，或在 experiment_config.json 的 api_key / 项目 .env 中填写。")
    return key


def call_deepseek(prompt: str, config: dict[str, Any], model: str | None = None) -> str:
    try:
        import requests
    except ModuleNotFoundError as exc:
        raise RuntimeError("缺少 requests 依赖，请先安装 requests 后再调用 DeepSeek API。") from exc

    headers = {
        "Authorization": f"Bearer {api_key(config)}",
        "Content-Type": "application/json",
    }
    payload = {
        "model": model or config["model"],
        "messages": [
            {
                "role": "system",
                "content": "你只输出用户要求的 JSON 对象本身，不要 markdown 代码围栏，不要前后解释。",
            },
            {"role": "user", "content": prompt},
        ],
        "max_tokens": int(config.get("max_tokens", 700)),
        "temperature": float(config.get("temperature", 0.7)),
    }
    response = requests.post(
        DEEPSEEK_API_URL,
        headers=headers,
        json=payload,
        timeout=float(config.get("request_timeout", 30)),
    )
    try:
        body = response.json()
    except json.JSONDecodeError as exc:
        raise RuntimeError(f"API 返回非 JSON: {response.status_code}, {response.text}") from exc
    if response.status_code != 200:
        raise RuntimeError(f"API Error: {response.status_code}, {body}")
    try:
        return body["choices"][0]["message"]["content"]
    except (KeyError, IndexError, TypeError) as exc:
        raise RuntimeError(f"Unexpected API response: {body}") from exc


def strip_json_fences(raw: str) -> str:
    text = raw.strip()
    match = re.fullmatch(r"```(?:json)?\s*([\s\S]*?)\s*```", text, re.IGNORECASE)
    if match:
        return match.group(1).strip()
    return text


def parse_json_object(raw: str) -> dict[str, Any]:
    text = strip_json_fences(raw)
    try:
        data = json.loads(text)
    except json.JSONDecodeError as exc:
        raise ValueError(f"JSON 解析失败: {exc.msg} at line {exc.lineno} column {exc.colno}") from exc
    if not isinstance(data, dict):
        raise ValueError("JSON 根节点必须是对象")
    return data


def validate_options(options: Any) -> list[dict[str, str]]:
    if not isinstance(options, list):
        raise ValueError("options 必须是数组")
    if len(options) != len(EXPECTED_LABELS):
        raise ValueError("options 必须正好包含 A/B/X/Y 四个选项")
    normalized: list[dict[str, str]] = []
    labels: list[str] = []
    for item in options:
        if not isinstance(item, dict):
            raise ValueError("每个 option 必须是对象")
        label = item.get("label")
        text = item.get("text")
        if not isinstance(label, str) or not isinstance(text, str):
            raise ValueError("option.label 和 option.text 必须是字符串")
        label = label.strip().upper()
        text = text.strip()
        if not text:
            raise ValueError("option.text 不能为空")
        labels.append(label)
        normalized.append({"label": label, "text": text})
    if sorted(labels) != sorted(EXPECTED_LABELS) or len(set(labels)) != len(labels):
        raise ValueError("options label 必须唯一且正好为 A/B/X/Y")
    return normalized


def parse_story_output(raw: str) -> dict[str, str]:
    data = parse_json_object(raw)
    story = data.get("story")
    if not isinstance(story, str) or not story.strip():
        raise ValueError("story 必须是非空字符串")
    return {"story": story.strip()}


def parse_options_output(raw: str) -> dict[str, Any]:
    data = parse_json_object(raw)
    return {"options": validate_options(data.get("options"))}


def parse_story_options_output(raw: str) -> dict[str, Any]:
    data = parse_json_object(raw)
    story = data.get("story")
    if not isinstance(story, str) or not story.strip():
        raise ValueError("story 必须是非空字符串")
    return {"story": story.strip(), "options": validate_options(data.get("options"))}


def parse_state_output(raw: str) -> dict[str, list[str]]:
    data = parse_json_object(raw)
    state: dict[str, list[str]] = {}
    for key in STATE_KEYS:
        value = data.get(key, [])
        if not isinstance(value, list) or not all(isinstance(x, str) for x in value):
            raise ValueError(f"{key} 必须是字符串数组")
        state[key] = [x.strip() for x in value if x.strip()]
    return state


def parse_judge_output(raw: str) -> dict[str, Any]:
    data = parse_json_object(raw)
    scores = data.get("scores")
    notes = data.get("notes")
    if not isinstance(scores, dict):
        raise ValueError("scores 必须是对象")
    required = ["continuity", "choice_quality", "style_match", "json_validity"]
    normalized_scores: dict[str, int] = {}
    for key in required:
        value = scores.get(key)
        if not isinstance(value, int) or not 1 <= value <= 5:
            raise ValueError(f"scores.{key} 必须是 1-5 的整数")
        normalized_scores[key] = value
    if not isinstance(notes, str):
        raise ValueError("notes 必须是字符串")
    return {"scores": normalized_scores, "notes": notes.strip()}


def context_payload(
    config: dict[str, Any],
    recent_story: list[str],
    state: dict[str, list[str]],
    last_choice: dict[str, str],
) -> dict[str, Any]:
    recent_turns = int(config.get("context", {}).get("recent_turns", 3))
    use_state = bool(config.get("context", {}).get("use_state", True))
    return {
        "world": config["world"],
        "style_ratio": config["style_ratio"],
        "recent_story": recent_story[-recent_turns:],
        "state": state if use_state else {},
        "last_choice": last_choice,
    }


def dumps_for_prompt(value: Any) -> str:
    return json.dumps(value, ensure_ascii=False, indent=2)


def prompt_story_segment(ctx: dict[str, Any]) -> str:
    return f"""
你是互动小说的 story_segment skill。
根据统一输入生成下一段故事。

统一输入：
{dumps_for_prompt(ctx)}

要求：
- 只推进一个清晰场景，不要提前列出选项。
- 故事段落 100-180 个中文字符。
- 延续 recent_story、state 和 last_choice。
- 输出 JSON：
{{"story":"..."}}
""".strip()


def prompt_options(ctx: dict[str, Any], story: str) -> str:
    return f"""
你是互动小说的 options skill。
根据故事生成 4 个分支选项。

统一输入：
{dumps_for_prompt(ctx)}

当前故事：
{story}

要求：
- 必须输出 A、B、X、Y 四个选项。
- 四个选项的行动方向必须明显不同。
- 每个选项 20-60 个中文字符。
- 输出 JSON：
{{"options":[{{"label":"A","text":"..."}},{{"label":"B","text":"..."}},{{"label":"X","text":"..."}},{{"label":"Y","text":"..."}}]}}
""".strip()


def prompt_style_apply(ctx: dict[str, Any], story: str, options: list[dict[str, str]]) -> str:
    return f"""
你是互动小说的 style_apply skill。
按 style_ratio 调整故事和选项，但不要改变核心事实、行动含义和选项标签。

统一输入：
{dumps_for_prompt(ctx)}

故事：
{story}

选项：
{dumps_for_prompt(options)}

要求：
- 输出统一故事结果 JSON。
- story 必须是非空字符串。
- options 必须正好包含 A、B、X、Y。
- 输出 JSON：
{{"story":"...","options":[{{"label":"A","text":"..."}},{{"label":"B","text":"..."}},{{"label":"X","text":"..."}},{{"label":"Y","text":"..."}}]}}
""".strip()


def prompt_story_options(ctx: dict[str, Any]) -> str:
    return f"""
你是互动小说的 story_options skill。
一次性生成下一段故事和 4 个分支选项。

统一输入：
{dumps_for_prompt(ctx)}

要求：
- 故事段落 90-150 个中文字符。
- 延续 recent_story、state 和 last_choice。
- options 必须正好包含 A、B、X、Y，且行动方向明显不同。
- 每个 option.text 控制在 18-45 个中文字符。
- 输出 JSON：
{{"story":"...","options":[{{"label":"A","text":"..."}},{{"label":"B","text":"..."}},{{"label":"X","text":"..."}},{{"label":"Y","text":"..."}}]}}
""".strip()


def prompt_state_update(
    ctx: dict[str, Any],
    displayed: dict[str, Any],
    user_choice: dict[str, str],
) -> str:
    return f"""
你是互动小说的 state_update skill。
根据本轮最终展示内容和玩家选择，更新结构化剧情状态。

上一轮上下文：
{dumps_for_prompt(ctx)}

最终展示：
{dumps_for_prompt(displayed)}

玩家选择：
{dumps_for_prompt(user_choice)}

要求：
- 只保留对后续剧情有用的稳定事实。
- 每个数组最多 8 条，短句表达。
- 输出 JSON：
{{"characters":[],"locations":[],"inventory":[],"open_threads":[],"facts":[]}}
""".strip()


def prompt_judge_turn(
    ctx: dict[str, Any],
    displayed: dict[str, Any],
    user_choice: dict[str, str],
    state: dict[str, list[str]],
) -> str:
    return f"""
你是互动小说实验的 judge_turn skill。
请评估本轮输出质量，用于比较提示词、skill 编排和上下文策略。

输入上下文：
{dumps_for_prompt(ctx)}

最终展示：
{dumps_for_prompt(displayed)}

玩家选择：
{dumps_for_prompt(user_choice)}

更新后状态：
{dumps_for_prompt(state)}

评分维度：
- continuity: 是否延续上下文
- choice_quality: 选项是否多样、可行动、有分支感
- style_match: 是否符合 world 和 style_ratio
- json_validity: 输出结构是否干净稳定

要求：
- 分数必须是 1-5 的整数。
- notes 用一句中文说明主要问题或亮点。
- 输出 JSON：
{{"scores":{{"continuity":5,"choice_quality":5,"style_match":5,"json_validity":5}},"notes":"..."}}
""".strip()


LLMCall = Callable[[str, dict[str, Any], str | None], str]
Parser = Callable[[str], dict[str, Any]]


def run_skill(
    name: str,
    prompt: str,
    config: dict[str, Any],
    parser: Parser,
    llm_call: LLMCall = call_deepseek,
    model: str | None = None,
) -> tuple[dict[str, Any] | None, dict[str, Any]]:
    started = time.perf_counter()
    event: dict[str, Any] = {
        "name": name,
        "prompt_version": PROMPT_VERSION,
        "prompt": prompt,
        "response_raw": None,
        "parsed": None,
        "latency_ms": None,
        "success": False,
        "error": None,
    }
    try:
        raw = llm_call(prompt, config, model)
        event["response_raw"] = raw
        parsed = parser(raw)
        event["parsed"] = parsed
        event["success"] = True
        return parsed, event
    except Exception as exc:
        event["error"] = str(exc)
        return None, event
    finally:
        event["latency_ms"] = round((time.perf_counter() - started) * 1000)


def run_three_step_pipeline(
    ctx: dict[str, Any],
    config: dict[str, Any],
    llm_call: LLMCall = call_deepseek,
) -> tuple[dict[str, Any] | None, list[dict[str, Any]]]:
    skill_events: list[dict[str, Any]] = []

    story, event = run_skill(
        "story_segment", prompt_story_segment(ctx), config, parse_story_output, llm_call
    )
    skill_events.append(event)
    if story is None:
        return None, skill_events

    options, event = run_skill(
        "options",
        prompt_options(ctx, story["story"]),
        config,
        parse_options_output,
        llm_call,
    )
    skill_events.append(event)
    if options is None:
        return None, skill_events

    displayed, event = run_skill(
        "style_apply",
        prompt_style_apply(ctx, story["story"], options["options"]),
        config,
        parse_story_options_output,
        llm_call,
    )
    skill_events.append(event)
    return displayed, skill_events


def run_single_step_pipeline(
    ctx: dict[str, Any],
    config: dict[str, Any],
    llm_call: LLMCall = call_deepseek,
) -> tuple[dict[str, Any] | None, list[dict[str, Any]]]:
    displayed, event = run_skill(
        "story_options", prompt_story_options(ctx), config, parse_story_options_output, llm_call
    )
    return displayed, [event]


def run_story_pipeline(
    ctx: dict[str, Any],
    config: dict[str, Any],
    llm_call: LLMCall = call_deepseek,
) -> tuple[dict[str, Any] | None, list[dict[str, Any]]]:
    if config["pipeline"] == "single_step":
        return run_single_step_pipeline(ctx, config, llm_call)
    return run_three_step_pipeline(ctx, config, llm_call)


def update_state(
    ctx: dict[str, Any],
    displayed: dict[str, Any],
    user_choice: dict[str, str],
    config: dict[str, Any],
    llm_call: LLMCall = call_deepseek,
) -> tuple[dict[str, list[str]], dict[str, Any] | None]:
    if not config.get("context", {}).get("use_state", True):
        return copy.deepcopy(EMPTY_STATE), None
    parsed, event = run_skill(
        "state_update",
        prompt_state_update(ctx, displayed, user_choice),
        config,
        parse_state_output,
        llm_call,
    )
    if parsed is None:
        return copy.deepcopy(ctx.get("state") or EMPTY_STATE), event
    return parsed, event


def judge_turn(
    ctx: dict[str, Any],
    displayed: dict[str, Any],
    user_choice: dict[str, str],
    state: dict[str, list[str]],
    config: dict[str, Any],
    llm_call: LLMCall = call_deepseek,
) -> dict[str, Any] | None:
    judge_config = config.get("judge", {})
    if not judge_config.get("enabled", False):
        return None
    _, event = run_skill(
        "judge_turn",
        prompt_judge_turn(ctx, displayed, user_choice, state),
        config,
        parse_judge_output,
        llm_call,
        model=judge_config.get("model") or None,
    )
    return event


def choose_option(displayed: dict[str, Any]) -> dict[str, str] | None:
    valid = {opt["label"]: opt["text"] for opt in displayed["options"]}
    choice_label = ""
    while choice_label not in valid:
        choice_label = input("请选择下一步 (A/B/X/Y, q 退出): ").strip().upper()
        if choice_label == "Q":
            return None
    return {"label": choice_label, "text": valid[choice_label]}


def print_display(displayed: dict[str, Any]) -> None:
    print("\n--- 当前故事 ---")
    print(displayed["story"])
    print("\n--- 选项 ---")
    for option in displayed["options"]:
        print(f"{option['label']}: {option['text']}")


def run_cli(config: dict[str, Any], llm_call: LLMCall = call_deepseek) -> None:
    recent_story: list[str] = []
    state = copy.deepcopy(EMPTY_STATE)
    last_choice = {"label": "", "text": ""}

    log_path = new_session_log_path()
    logger = ConversationSessionLog(log_path)
    print("=== 互动小说 Skill 编排实验台 (DeepSeek API) ===")
    print(f"对话日志文件: {os.path.abspath(log_path)}")
    print(f"pipeline: {config['pipeline']}, prompt_version: {PROMPT_VERSION}")

    try:
        logger.write_event(
            {
                "type": "session_meta",
                "ts_utc": utc_now_iso(),
                "prompt_version": PROMPT_VERSION,
                "config": config_for_log(config),
            }
        )

        turn_index = 0
        while True:
            turn_index += 1
            turn_started = utc_now_iso()
            ctx = context_payload(config, recent_story, state, last_choice)
            displayed, skill_events = run_story_pipeline(ctx, config, llm_call)

            if displayed is None:
                logger.write_event(
                    {
                        "type": "turn_failed",
                        "turn_index": turn_index,
                        "ts_utc": turn_started,
                        "phase": "story_pipeline",
                        "context": ctx,
                        "skills": skill_events,
                    }
                )
                print("故事生成失败，已写入日志。")
                break

            print_display(displayed)
            user_choice = choose_option(displayed)
            if user_choice is None:
                logger.write_event(
                    {
                        "type": "turn_abandoned",
                        "turn_index": turn_index,
                        "ts_utc": turn_started,
                        "reason": "user_quit_after_display",
                        "context": ctx,
                        "skills": skill_events,
                        "displayed": displayed,
                    }
                )
                logger.write_event(
                    {
                        "type": "session_end",
                        "reason": "user_quit",
                        "ts_utc": utc_now_iso(),
                        "turn_index": turn_index,
                    }
                )
                print("退出互动小说。")
                return

            recent_story.append(displayed["story"])
            state, state_event = update_state(ctx, displayed, user_choice, config, llm_call)
            judge_event = judge_turn(ctx, displayed, user_choice, state, config, llm_call)
            last_choice = user_choice

            all_skill_events = list(skill_events)
            if state_event:
                all_skill_events.append(state_event)
            if judge_event:
                all_skill_events.append(judge_event)

            logger.write_event(
                {
                    "type": "turn",
                    "turn_index": turn_index,
                    "ts_utc": turn_started,
                    "context": ctx,
                    "skills": all_skill_events,
                    "displayed": displayed,
                    "user_choice": user_choice,
                    "state": state,
                    "judge": judge_event.get("parsed") if judge_event else None,
                }
            )
    finally:
        logger.close()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="互动小说 Skill 编排实验台")
    parser.add_argument(
        "--config",
        default=DEFAULT_CONFIG_PATH,
        help=f"实验配置 JSON 路径，默认 {DEFAULT_CONFIG_PATH}",
    )
    parser.add_argument(
        "--pipeline",
        choices=["three_step", "single_step"],
        help="临时覆盖配置中的 pipeline",
    )
    parser.add_argument(
        "--fast",
        action="store_true",
        help="启用快速模式：单步生成、关闭状态更新和 judge、减少上下文",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    config = load_config(args.config)
    if args.fast:
        config["pipeline"] = "single_step"
        config["max_tokens"] = min(int(config.get("max_tokens", 900)), 900)
        config.setdefault("context", {})["recent_turns"] = min(
            int(config.get("context", {}).get("recent_turns", 2)), 2
        )
        config.setdefault("context", {})["use_state"] = False
        config.setdefault("judge", {})["enabled"] = False
    if args.pipeline:
        config["pipeline"] = args.pipeline
    validate_config(config)
    run_cli(config)


if __name__ == "__main__":
    main()
