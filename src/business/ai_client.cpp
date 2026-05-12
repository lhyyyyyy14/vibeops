#include "business/ai_client.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#define POPEN _popen
#define PCLOSE _pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif

namespace {
std::string ReadFile(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  std::ostringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

void WriteFile(const std::string &path, const std::string &content) {
  std::ofstream file(path, std::ios::binary);
  file << content;
}

void AppendLog(const std::string &line) {
  std::ofstream file("gb_internovel_runtime.log", std::ios::app | std::ios::binary);
  file << line << "\n";
}

std::string Preview(const std::string &text, std::size_t limit = 600) {
  std::string out = text.substr(0, std::min(limit, text.size()));
  for (char &c : out) {
    if (c == '\r' || c == '\n' || c == '\t') c = ' ';
  }
  return out;
}

bool FileExists(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  return file.good();
}

std::string JsonEscape(const std::string &text) {
  std::string out;
  for (unsigned char c : text) {
    switch (c) {
    case '\\': out += "\\\\"; break;
    case '"': out += "\\\""; break;
    case '\n': out += "\\n"; break;
    case '\r': out += "\\r"; break;
    case '\t': out += "\\t"; break;
    default:
      if (c < 0x20) {
        char buf[8]{};
        std::snprintf(buf, sizeof(buf), "\\u%04x", c);
        out += buf;
      } else {
        out.push_back(static_cast<char>(c));
      }
    }
  }
  return out;
}

std::string ShellQuote(const std::string &path) {
#if defined(_WIN32)
  std::string out = "\"";
  for (char c : path) {
    if (c == '"') out += "\\\"";
    else out += c;
  }
  out += "\"";
  return out;
#else
  std::string out = "'";
  for (char c : path) {
    if (c == '\'') out += "'\\''";
    else out += c;
  }
  out += "'";
  return out;
#endif
}

std::string TempPath(const std::string &suffix) {
#if defined(_WIN32)
  return ".\\gb_internovel_" + std::to_string(std::rand()) + suffix;
#else
  const char *tmp = std::getenv("TMPDIR");
  std::string base = tmp && *tmp ? tmp : ".";
  if (!base.empty() && base.back() != '/' && base.back() != '\\') base += "/";
  return base + "gb_internovel_" + std::to_string(std::rand()) + suffix;
#endif
}

std::string ReadCommand(const std::string &command, int &exit_code) {
#if defined(_WIN32)
  SECURITY_ATTRIBUTES security{};
  security.nLength = sizeof(security);
  security.bInheritHandle = TRUE;

  HANDLE read_pipe = nullptr;
  HANDLE write_pipe = nullptr;
  if (!CreatePipe(&read_pipe, &write_pipe, &security, 0)) {
    exit_code = -1;
    return "";
  }
  SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFOA startup{};
  startup.cb = sizeof(startup);
  startup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
  startup.wShowWindow = SW_HIDE;
  startup.hStdOutput = write_pipe;
  startup.hStdError = write_pipe;
  startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

  PROCESS_INFORMATION process{};
  std::string command_line = "cmd.exe /C " + command;
  std::vector<char> mutable_command(command_line.begin(), command_line.end());
  mutable_command.push_back('\0');

  const BOOL started =
      CreateProcessA(nullptr, mutable_command.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr,
                     &startup, &process);
  CloseHandle(write_pipe);

  if (!started) {
    CloseHandle(read_pipe);
    exit_code = -1;
    return "";
  }

  std::array<char, 4096> buffer{};
  std::string output;
  DWORD bytes_read = 0;
  while (::ReadFile(read_pipe, buffer.data(), static_cast<DWORD>(buffer.size()), &bytes_read, nullptr) &&
         bytes_read > 0) {
    output.append(buffer.data(), bytes_read);
  }
  CloseHandle(read_pipe);
  WaitForSingleObject(process.hProcess, INFINITE);

  DWORD process_exit = 0;
  GetExitCodeProcess(process.hProcess, &process_exit);
  CloseHandle(process.hThread);
  CloseHandle(process.hProcess);
  exit_code = static_cast<int>(process_exit);
  return output;
#else
  std::array<char, 4096> buffer{};
  std::string output;
  FILE *pipe = POPEN(command.c_str(), "r");
  if (!pipe) {
    exit_code = -1;
    return "";
  }
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe)) {
    output += buffer.data();
  }
  exit_code = PCLOSE(pipe);
  return output;
#endif
}

std::string ExtractJsonStringAt(const std::string &json, std::size_t quote_pos, std::size_t *end_pos = nullptr) {
  std::string out;
  if (quote_pos == std::string::npos || quote_pos >= json.size() || json[quote_pos] != '"') return out;
  for (std::size_t i = quote_pos + 1; i < json.size(); ++i) {
    const char c = json[i];
    if (c == '"') {
      if (end_pos) *end_pos = i + 1;
      return out;
    }
    if (c != '\\') {
      out.push_back(c);
      continue;
    }
    if (++i >= json.size()) break;
    const char esc = json[i];
    switch (esc) {
    case '"': out.push_back('"'); break;
    case '\\': out.push_back('\\'); break;
    case '/': out.push_back('/'); break;
    case 'b': out.push_back('\b'); break;
    case 'f': out.push_back('\f'); break;
    case 'n': out.push_back('\n'); break;
    case 'r': out.push_back('\r'); break;
    case 't': out.push_back('\t'); break;
    case 'u': {
      if (i + 4 >= json.size()) break;
      unsigned int code = 0;
      for (int j = 0; j < 4; ++j) {
        const char h = json[i + 1 + j];
        code <<= 4;
        if (h >= '0' && h <= '9') code += h - '0';
        else if (h >= 'a' && h <= 'f') code += h - 'a' + 10;
        else if (h >= 'A' && h <= 'F') code += h - 'A' + 10;
      }
      if (code <= 0x7F) {
        out.push_back(static_cast<char>(code));
      } else if (code <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | ((code >> 6) & 0x1F)));
        out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
      } else {
        out.push_back(static_cast<char>(0xE0 | ((code >> 12) & 0x0F)));
        out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
      }
      i += 4;
      break;
    }
    default: out.push_back(esc); break;
    }
  }
  return out;
}

std::string ExtractStringValue(const std::string &json, const std::string &key, std::size_t start = 0,
                               std::size_t *end_pos = nullptr) {
  const std::string needle = "\"" + key + "\"";
  std::size_t key_pos = json.find(needle, start);
  if (key_pos == std::string::npos) return "";
  std::size_t colon = json.find(':', key_pos + needle.size());
  if (colon == std::string::npos) return "";
  std::size_t quote = json.find('"', colon + 1);
  return ExtractJsonStringAt(json, quote, end_pos);
}

int ExtractIntValue(const std::string &json, const std::string &key, int fallback) {
  const std::string needle = "\"" + key + "\"";
  std::size_t key_pos = json.find(needle);
  if (key_pos == std::string::npos) return fallback;
  std::size_t colon = json.find(':', key_pos + needle.size());
  if (colon == std::string::npos) return fallback;
  return std::atoi(json.c_str() + colon + 1);
}

double ExtractDoubleValue(const std::string &json, const std::string &key, double fallback) {
  const std::string needle = "\"" + key + "\"";
  std::size_t key_pos = json.find(needle);
  if (key_pos == std::string::npos) return fallback;
  std::size_t colon = json.find(':', key_pos + needle.size());
  if (colon == std::string::npos) return fallback;
  return std::atof(json.c_str() + colon + 1);
}

std::array<Choice, 4> EmptyChoices() {
  return {Choice{'A', ""}, Choice{'B', ""}, Choice{'X', ""}, Choice{'Y', ""}};
}

AiTurnResult ParseStoryOptions(const std::string &content) {
  AiTurnResult result;
  result.choices = EmptyChoices();
  result.story = ExtractStringValue(content, "story");
  if (result.story.empty()) {
    result.error = "API 返回中缺少 story";
    return result;
  }

  std::size_t pos = 0;
  int found = 0;
  while (found < 4) {
    std::size_t label_key = content.find("\"label\"", pos);
    if (label_key == std::string::npos) break;
    std::size_t label_end = 0;
    std::string label = ExtractStringValue(content, "label", label_key, &label_end);
    std::size_t text_end = 0;
    std::string text = ExtractStringValue(content, "text", label_end, &text_end);
    if (!label.empty() && !text.empty()) {
      char c = static_cast<char>(std::toupper(static_cast<unsigned char>(label[0])));
      if (c == 'A' || c == 'B' || c == 'X' || c == 'Y') {
        const int index = c == 'A' ? 0 : c == 'B' ? 1 : c == 'X' ? 2 : 3;
        result.choices[static_cast<std::size_t>(index)] = Choice{c, text};
        ++found;
      }
    }
    pos = text_end ? text_end : label_key + 7;
  }

  for (const Choice &choice : result.choices) {
    if (choice.text.empty()) {
      result.error = "API 返回中缺少 A/B/X/Y 四个选项";
      return result;
    }
  }
  result.ok = true;
  return result;
}

std::string BuildPrompt(const AiConfig &config, const AiTurnRequest &request) {
  std::ostringstream prompt;
  prompt << "你是互动小说的 story_options skill。一次性生成下一段故事和 4 个分支选项。\n";
  prompt << "世界观：" << config.world << "\n";
  prompt << "本局关键词：" << JoinSetupLabels(request.setup.keywords) << "\n";
  prompt << "本局风格：" << JoinSetupLabels(request.setup.styles) << "\n";
  prompt << "最近剧情：";
  if (request.recent_story.empty()) {
    prompt << "[]";
    prompt << "\n这是第一轮，请根据关键词和风格生成一个清晰、有钩子的开场故事。";
  } else {
    for (const std::string &story : request.recent_story) prompt << "\n- " << story;
    prompt << "\n后续剧情必须延续本局关键词和风格，不要突然切换题材。";
  }
  prompt << "\n上一步选择：";
  if (request.has_last_choice) {
    prompt << request.last_choice.label << "：" << request.last_choice.text;
  } else {
    prompt << "无";
  }
  prompt << "\n要求：故事 90-150 个中文字符；每个 option.text 18-45 个中文字符；";
  prompt << "options 必须正好包含 A、B、X、Y 且行动方向明显不同；";
  prompt << "只输出 JSON：{\"story\":\"...\",\"options\":[{\"label\":\"A\",\"text\":\"...\"},{\"label\":\"B\",\"text\":\"...\"},{\"label\":\"X\",\"text\":\"...\"},{\"label\":\"Y\",\"text\":\"...\"}]}";
  return prompt.str();
}

std::string BuildPayload(const AiConfig &config, const std::string &prompt) {
  std::ostringstream payload;
  payload << "{";
  payload << "\"model\":\"" << JsonEscape(config.model) << "\",";
  payload << "\"messages\":[";
  payload << "{\"role\":\"system\",\"content\":\"你只输出用户要求的 JSON 对象本身，不要 markdown 代码围栏，不要前后解释。\"},";
  payload << "{\"role\":\"user\",\"content\":\"" << JsonEscape(prompt) << "\"}],";
  payload << "\"max_tokens\":" << config.max_tokens << ",";
  payload << "\"temperature\":" << config.temperature;
  payload << "}";
  return payload.str();
}

std::string CurlCommand(const AiConfig &config, const std::string &request_path) {
  std::ostringstream cmd;
#if defined(_WIN32)
  const std::string curl = FileExists(".\\curl.exe") ? ".\\curl.exe" : "curl.exe";
#else
  const std::string curl = FileExists("./curl") ? "./curl" : "curl";
#endif
#if defined(_WIN32)
  cmd << curl << " -sS --globoff --max-time " << config.request_timeout;
#else
  cmd << ShellQuote(curl) << " -sS --globoff --max-time " << config.request_timeout;
#endif
  if (FileExists("ca-bundle.crt")) {
    cmd << " --cacert " << ShellQuote("ca-bundle.crt");
  }
  cmd << " -H " << ShellQuote("Authorization: Bearer " + config.api_key);
  cmd << " -H " << ShellQuote("Content-Type: application/json");
  cmd << " --data-binary " << ShellQuote("@" + request_path);
  cmd << " " << ShellQuote(config.api_url);
  cmd << " 2>&1";
  return cmd.str();
}
}  // namespace

AiConfig LoadAiConfig(const std::string &path) {
  AiConfig config;
  const std::string text = ReadFile(path);
  if (!text.empty()) {
    std::string model = ExtractStringValue(text, "model");
    std::string api_key = ExtractStringValue(text, "api_key");
    std::string world = ExtractStringValue(text, "world");
    if (!model.empty()) config.model = model;
    if (!api_key.empty()) config.api_key = api_key;
    if (!world.empty()) config.world = world;
    config.max_tokens = ExtractIntValue(text, "max_tokens", config.max_tokens);
    config.request_timeout = ExtractIntValue(text, "request_timeout", config.request_timeout);
    config.temperature = ExtractDoubleValue(text, "temperature", config.temperature);
  }
  if (const char *env_key = std::getenv("DEEPSEEK_API_KEY")) {
    if (*env_key) config.api_key = env_key;
  }
  if (config.api_key.empty()) {
    const std::string env_text = ReadFile(".env");
    std::size_t pos = env_text.find("DEEPSEEK_API_KEY=");
    if (pos != std::string::npos) {
      pos += 17;
      std::size_t end = env_text.find_first_of("\r\n", pos);
      config.api_key = env_text.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
    }
  }
  return config;
}

AiTurnResult GenerateStoryTurn(const AiConfig &config, const AiTurnRequest &request) {
  AiTurnResult result;
  result.choices = EmptyChoices();
  result.prompt = BuildPrompt(config, request);
  if (config.api_key.empty()) {
    result.error = "缺少 API Key：请在 experiment_config.json、.env 或环境变量中设置";
    return result;
  }

  const std::string request_path = TempPath("_request.json");
  WriteFile(request_path, BuildPayload(config, result.prompt));
  int exit_code = 0;
  const std::string command = CurlCommand(config, request_path);
  AppendLog("[api] request start");
  const auto started = std::chrono::steady_clock::now();
  const std::string response = ReadCommand(command, exit_code);
  result.latency_ms = static_cast<int>(
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count());
  result.raw_response = response;
  AppendLog("[api] exit=" + std::to_string(exit_code) + " bytes=" + std::to_string(response.size()));
  if (!response.empty()) AppendLog("[api] response=" + Preview(response));
  std::remove(request_path.c_str());
  if (exit_code != 0 || response.empty()) {
    result.error = response.empty() ? "curl 请求失败：没有收到响应" : "curl 请求失败：" + Preview(response, 180);
    return result;
  }

  const std::string content = ExtractStringValue(response, "content");
  if (content.empty()) {
    result.error = "API 响应缺少 choices.message.content";
    return result;
  }
  result = ParseStoryOptions(content);
  result.prompt = BuildPrompt(config, request);
  result.raw_response = response;
  result.latency_ms = static_cast<int>(
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count());
  if (!result.ok && result.error.empty()) result.error = "无法解析 API 返回的故事 JSON";
  return result;
}
