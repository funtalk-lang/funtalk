#include "tokenizer.h"
#include <stack>
#include <string>
#include <vector>
#include <cctype>

std::vector<std::string> split(const std::string& str) {
    std::vector<std::string> tokens;
    size_t i = 0;
    size_t n = str.length();
    while (i < n) {
        if (std::isspace(static_cast<unsigned char>(str[i])) && str[i] != '\n') {
            i++;
            continue;
        }
        char ch = str[i];
        if (ch == '"' || ch == '\'') {
            char quote = ch;
            size_t start = i;
            i++;
            while (i < n && str[i] != quote) {
                if (str[i] == '\\' && i + 1 < n) {
                    i += 2;
                } else {
                    i++;
                }
            }
            if (i < n) i++;
            tokens.push_back(str.substr(start, i - start));
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(ch)) || (ch == '-' && i + 1 < n && std::isdigit(static_cast<unsigned char>(str[i + 1])))) {
            size_t start = i;
            i++;
            while (i < n && std::isdigit(static_cast<unsigned char>(str[i]))) {
                i++;
            }
            if (i + 1 < n && str[i] == '.' && std::isdigit(static_cast<unsigned char>(str[i + 1]))) {
                i += 2;
                while (i < n && std::isdigit(static_cast<unsigned char>(str[i]))) {
                    i++;
                }
            }
            tokens.push_back(str.substr(start, i - start));
            continue;
        }
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_') {
            size_t start = i;
            while (i < n && (std::isalnum(static_cast<unsigned char>(str[i])) || str[i] == '_')) {
                i++;
            }
            tokens.push_back(str.substr(start, i - start));
            continue;
        }
        if (ch == '=' || ch == '{' || ch == '}' || ch == '[' || ch == ']' || ch == '(' || ch == ')' || ch == ',' || ch == '.' || ch == '\n') {
            tokens.push_back(std::string(1, ch));
            i++;
            continue;
        }
        i++;
    }
    return tokens;
}

bool checkBrackets(const std::string& text) {
    std::stack<char> s;
    for (char ch : text) {
        if (ch == '(' || ch == '[' || ch == '{') {
            s.push(ch);
        }
        else if (ch == ')' || ch == ']' || ch == '}') {
            if (s.empty()) return false;
            char top = s.top();
            if ((ch == ')' && top == '(') || (ch == ']' && top == '[') || (ch == '}' && top == '{')) {
                s.pop();
            } else {
                return false;
            }
        }
        else if (ch == ',') {
            if (s.empty()) return false;
            char current_bracket = s.top();
            if (current_bracket == '{') return false;
        }
        else if (ch == '\n') {
            if (s.empty()) continue;
            char current_bracket = s.top();
        }
    }
    return s.empty();
}

std::string processText(std::string text) {
    bool inComment = false;
    bool inDoubleQuotes = false;
    bool inSingleQuotes = false;
    bool escaped = false;
    std::string result = "";
    for (char c : text) {
        if (escaped) {
            if (!inComment) result += c;
            escaped = false;
            continue;
        }
        if (c == '\\') {
            if (!inComment) result += c;
            escaped = true;
            continue;
        }
        if (inDoubleQuotes) {
            if (c == '"') inDoubleQuotes = false;
            result += c;
        }
        else if (inSingleQuotes) {
            if (c == '\'') inSingleQuotes = false;
            result += c;
        }
        else if (inComment) {
            if (c == '\n') {
                inComment = false;
                result += c;
            }
        }
        else {
            if (c == '"') {
                inDoubleQuotes = true;
                result += c;
            } else if (c == '\'') {
                inSingleQuotes = true;
                result += c;
            } else if (c == ';') {
                inComment = true;
            } else if (c == ',') {
                result += '\n';
            } else {
                result += c;
            }
        }
    }
    return std::move(result);
}
