#include "tokenizer.h"

std::string tokenTypeToString(TokenType type) {
    static const std::string type_strings[] = {
        "BOOL", "INT", "FLOAT", "CHAR", "STRING",
        "LOWER_NAME", "SNAKE_NAME", "UPPER_NAME", "NONLOCAL_MESSAGE", "MESSAGE",
        "CODE_OPEN", "CODE_CLOSE",
        "ARR_OPEN", "ARR_CLOSE",
        "TUPLE_OPEN", "TUPLE_CLOSE",
        "MESSAGE_OPEN", "MESSAGE_CLOSE",
        "ASS", "DOT", "CALL", "SEND", "NEWLINE", "MOV", "FUNC_DECL", "MESSAGE_DECL"
    };
    return type_strings[static_cast<int>(type)];
}

std::string blockTypeToString(TokenType type) {
    static const std::string type_strings[] = {
        "object block",
        "array block",
        "tuple block",
        "message block",
    };
    return type_strings[(static_cast<int>(type) - 8) / 2];
}

void printToken(const Token& token) {
    std::string value_str = token.value;
    std::cout << "Token type: " << tokenTypeToString(token.type) << ", value: \"" << value_str << "\"\n";
}

TokenType determineIdentifierType(const std::string& str) {
    if (str == "TRUE" || str == "FALSE") return TokenType::BOOL;

    bool has_upper = std::any_of(str.begin(), str.end(), ::isupper);
    bool has_lower = std::any_of(str.begin(), str.end(), ::islower);
    bool has_under = str.find('_') != std::string::npos;

    if (has_upper && !has_lower) return TokenType::UPPER_NAME;
    if (has_under && has_lower) return TokenType::SNAKE_NAME;
    return TokenType::LOWER_NAME;
}

TokenType determineLiteralType(const std::string& str) {
    if (str == "=") return TokenType::ASS;
    if (str == ".") return TokenType::DOT;
    if (str == "\n") return TokenType::NEWLINE;

    if (str[0] == '\'') return TokenType::CHAR;
    if (str[0] == '"')  return TokenType::STRING;

    size_t start = (str[0] == '-' && str.length() > 1) ? 1 : 0;
    size_t dot_count = std::count(str.begin() + start, str.end(), '.');
    bool all_digits = std::all_of(str.begin() + start, str.end(), [](char c) {
        return std::isdigit(c) || c == '.';
    });

    if (all_digits && dot_count <= 1 && str.length() > start + dot_count) {
        return (dot_count == 1) ? TokenType::FLOAT : TokenType::INT;
    }

    if (str == "{") return TokenType::CODE_OPEN;
    if (str == "[") return TokenType::ARR_OPEN;
    if (str == "(") return TokenType::TUPLE_OPEN;
    if (str == "}") return TokenType::CODE_CLOSE;
    if (str == "]") return TokenType::ARR_CLOSE;
    if (str == ")") return TokenType::TUPLE_CLOSE;

    throw TokenizerPanic("Impossible TokenType of value: " + str);
}

std::vector<Token> tokenize(const std::vector<std::string>& v) {
    if (v.size() == 0) {
        throw FunTalkTokenizationError("Nothing to tokenize");
    }
    std::vector<Token> tokens;
    tokens.reserve(v.size());

    for (const auto& s : v) {
        if (s.empty()) continue;

        TokenType type;
        if (std::isalpha(s[0]) || s[0] == '_') {
            type = determineIdentifierType(s);
        } else {
            type = determineLiteralType(s);
        }

        tokens.push_back({type, s});
    }
    return tokens;
}

bool isOpenBracket(TokenType type) {
    return type == TokenType::CODE_OPEN || type == TokenType::ARR_OPEN || type == TokenType::TUPLE_OPEN || type == TokenType::MESSAGE_OPEN;
}

bool isCloseBracket(TokenType type) {
    return type == TokenType::CODE_CLOSE || type == TokenType::ARR_CLOSE || type == TokenType::TUPLE_CLOSE || type == TokenType::MESSAGE_CLOSE;
}

const Token* findBlockClose(const Token* begin, const Token* end) {
    if (begin >= end) {
        throw TokenizerPanic("Invalid data in findBlockClose");
    }
    TokenType type = TokenType(int(begin->type) + 1);
    for (const Token* p = begin + 1; p != end; p++) {
        if (p->type == type) {
            return p;
        }
        if (isOpenBracket(p->type)) {
            p = findBlockClose(p, end);
        }
    }
    throw TokenizerPanic("This error cannot be real if you checked your brackets before split. You also must call processText after checkBrackets: " + tokenTypeToString(type));
}

bool checkMessageDeclTuple(const Token* begin, const Token* end) {
    bool prevIsName = false;
    for (const Token* p = begin + 1; p != end; p++) {
        if (prevIsName) {
            if (p->type != TokenType::NEWLINE) {
                return false;
            }
            prevIsName = false;
        } else {
            prevIsName = p->type != TokenType::NEWLINE;
        }
    }
    return true;
}

void addMessageDeclArgs(std::vector<Token>& dest, const Token* begin, const Token* end) {
    for (const Token* p = begin + 1; p != end; p++) {
        if (p->type != TokenType::NEWLINE) {
            dest.push_back(*p);
        }
    }
}

void check(const Token* p, const Token* end) {
    if (p >= end) {
        throw TokenizerPanic("Invalid Token* p; p >= end");
    }
}

void fixTokens1(std::vector<Token>& dest, const Token* begin, const Token* end) {
    for (const Token* p = begin; p != end; p++) {
        check(p, end);
        if (p != end && p->type >= TokenType::LOWER_NAME && p->type <= TokenType::UPPER_NAME) {
            if ((p + 1) != end && p[1].type == TokenType::DOT) {
                if ((p + 2) == end || !(p[2].type >= TokenType::LOWER_NAME && p[2].type <= TokenType::UPPER_NAME)) {
                    throw FunTalkTokenizationError("Cannot send message without name after dot");
                }
                if (p->type == TokenType::UPPER_NAME) {
                    throw FunTalkTokenizationError("Cannot use constant before dot");
                }
                if ((p + 3) == end || p[3].type != TokenType::TUPLE_OPEN) {
                    throw FunTalkTokenizationError("Cannot send message without arguments in parentheses");
                }
                const Token* blockEnd = findBlockClose(p + 3, end);
                if ((blockEnd + 1) == end || blockEnd[1].type != TokenType::NEWLINE) {
                    throw FunTalkTokenizationError("Message cannot return");
                }
                dest.push_back(Token(TokenType::MESSAGE, p[0].value + "." + p[2].value));
                p += 2;
                continue;
            }
        }
        if (p != end && p->type == TokenType::DOT) {
            if ((p + 1) == end || !(p[1].type >= TokenType::LOWER_NAME && p[1].type <= TokenType::UPPER_NAME)) {
                throw FunTalkTokenizationError("Cannot send message without name after dot");
            }
            if ((p + 2) == end || p[2].type != TokenType::TUPLE_OPEN) {
                throw FunTalkTokenizationError("Cannot send message without arguments in parentheses");
            }
            const Token* blockEnd = findBlockClose(p + 2, end);
            if ((blockEnd + 1) == end || blockEnd[1].type != TokenType::NEWLINE) {
                throw FunTalkTokenizationError("Message cannot return");
            }
            dest.push_back(Token(TokenType::NONLOCAL_MESSAGE, p[1].value));
            p++;
            continue;
        }
        int i = 0;
        while ((p + i) != end && p[i].type == TokenType::NEWLINE) {
            i++;
        }
        if (i > 0) {
            dest.push_back(Token(TokenType::NEWLINE, ""));
            p += i - 1;
            continue;
        }
        dest.push_back(*p);
    }
    if ((&*dest.end() - 1)->type != TokenType::NEWLINE) {
        dest.push_back(Token(TokenType::NEWLINE, ""));
    }
}

void fixTokens2(std::vector<Token>& dest, const Token* begin, const Token* end, TokenType blockType) {
    bool hasAss = false;
    bool hasSend = false;
    if (begin->type == TokenType::NEWLINE) {
        begin++;
    }
    for (const Token* p = begin; p != end; p++) {
        check(p, end);
        if (p->type == TokenType::NEWLINE) {
            if (blockType == TokenType::ARR_OPEN && hasAss) throw FunTalkTokenizationError("Cannot assign in array block");
            if (blockType == TokenType::ARR_OPEN && hasSend) throw FunTalkTokenizationError("Cannot send in array block");
            if (blockType == TokenType::TUPLE_OPEN && hasAss) throw FunTalkTokenizationError("Cannot assign in tuple block");
            if (blockType == TokenType::TUPLE_OPEN && hasSend) throw FunTalkTokenizationError("Cannot send in tuple block");
            hasAss = false;
            hasSend = false;
            dest.push_back(*p);
        } else if (p->type == TokenType::ASS) {
            if (hasAss) {
                throw FunTalkTokenizationError("Cannot use multiple assign in the same line");
            }
            if (hasSend) {
                throw FunTalkTokenizationError("Cannot use assign and dot in the same line");
            }
            hasAss = true;
            dest.push_back(*p);
        } else if (p->type == TokenType::NONLOCAL_MESSAGE || p->type == TokenType::MESSAGE) {
            if (hasAss) {
                throw FunTalkTokenizationError("Cannot use multiple dot in the same line");
            }
            if (hasSend) {
                throw FunTalkTokenizationError("Cannot use assign and dot in the same line");
            }
            check(p + 2, end);
            if (!((p[1].type >= TokenType::LOWER_NAME && p[1].type <= TokenType::UPPER_NAME) || isOpenBracket(p[1].type))) {
                throw FunTalkTokenizationError("Invalid message argument");
            }
            hasSend = true;
            dest.push_back(Token(TokenType::SEND, ""));
            dest.push_back(*p);
            const Token* blockEnd = findBlockClose(p + 1, end);
            dest.push_back(p[1]);
            fixTokens2(dest, p + 2, blockEnd, p[2].type);
            dest.push_back(*blockEnd);
            p = blockEnd;
        } else if (isOpenBracket(p->type)) {
            check(p + 1, end);
            const Token* blockEnd = findBlockClose(p, end);
            dest.push_back(*p);
            fixTokens2(dest, p + 1, blockEnd, p->type);
            p = blockEnd;
            dest.push_back(*p);
        } else {
            dest.push_back(*p);
        }
    }
}

void fixTokens3(std::vector<Token>& dest, const Token* begin, const Token* end, TokenType blockType) {
    for (const Token* p = begin; p != end; p++) {
        check(p, end);
        int i = 0;
        while ((p + i) != end && p[i].type >= TokenType::LOWER_NAME && p[i].type <= TokenType::UPPER_NAME) {
            i++;
        }
        if (i > 1 && (p + i) != end && p[i].type == TokenType::ASS) {
            dest.push_back(Token(TokenType::FUNC_DECL, ""));
            for (int j = 0; j <= i; j++) {
                dest.push_back(p[j]);
            }
            p += i;
        } else if (i == 1) {
            if ((p + 1) == end) {
                dest.push_back(*p);
            } else if (p[1].type == TokenType::TUPLE_OPEN) {
                const Token* blockEnd = findBlockClose(p + 1, end);
                if (checkMessageDeclTuple(p + 1, blockEnd) && (blockEnd + 1) != end && blockEnd[1].type == TokenType::ASS) {
                    if ((blockEnd + 2) == end || blockEnd[2].type != TokenType::CODE_OPEN) {
                        throw FunTalkTokenizationError("Cannot declare message without message body");
                    }
                    dest.push_back(Token(TokenType::MESSAGE_DECL, ""));
                    dest.push_back(p[0]);
                    addMessageDeclArgs(dest, p + 1, blockEnd);
                    dest.push_back(Token(TokenType::MESSAGE_OPEN, ""));
                    const Token* blockEnd1 = findBlockClose(blockEnd + 2, end);
                    fixTokens3(dest, blockEnd + 3, blockEnd1, TokenType::MESSAGE_OPEN);
                    p = blockEnd1;
                    dest.push_back(Token(TokenType::MESSAGE_CLOSE, ""));
                } else {
                    dest.push_back(p[0]);
                    dest.push_back(p[1]);
                    fixTokens3(dest, p + 2, blockEnd, TokenType::TUPLE_OPEN);
                    dest.push_back(*blockEnd);
                    p = blockEnd;
                }
            } else if (p[1].type == TokenType::ASS) {
                dest.push_back(Token(TokenType::MOV, ""));
                dest.push_back(p[0]);
                dest.push_back(p[2]);
                p += 2;
            } else {
                dest.push_back(*p);
            }
        } else if (isOpenBracket(p->type)) {
            const Token* blockEnd = findBlockClose(p, end);
            dest.push_back(*p);
            fixTokens3(dest, p + 1, blockEnd, p->type);
            dest.push_back(*blockEnd);
            p = blockEnd;
        } else {
            if (p->type == TokenType::SEND && blockType == TokenType::CODE_OPEN) {
                throw FunTalkTokenizationError("Cannot send in object init block");
            }
            dest.push_back(*p);
        }
    }
}

int countCallSequence(const Token* begin, const Token* end) {
    int count = 0;
    for (const Token* p = begin; ; p++) {
        if (isOpenBracket(p->type)) {
            p = findBlockClose(p, end);
        } else if (p->type >= TokenType::BOOL && p->type <= TokenType::UPPER_NAME) {

        } else if (p->type == TokenType::NEWLINE || isCloseBracket(p->type)) {
            return count;
        } else {
            throw TokenizerPanic("This error cannot be real in countCallSequence 1: " + tokenTypeToString(p[0].type));
        }
        count++;
    }
    throw TokenizerPanic("This error cannot be real in countCallSequence 2");
}

bool hasComma(const Token* begin, const Token* end) {
    for (const Token* p = begin; p != end; p++) {
        if (p->type == TokenType::NEWLINE) {
            return true;
        }
        if (isOpenBracket(p->type)) {
            p = findBlockClose(p, end);
        }
    }
    return false;
}

void simplifyBlockExpression(std::vector<Token>& dest, const Token* begin, const Token* end) {
    for (const Token* p = begin; p != end; p++) {
        if (isOpenBracket(p->type)) {
            const Token* blockEnd = findBlockClose(p, end);
            if ((p + 1) == blockEnd) {
                dest.push_back(*p);
                dest.push_back(*blockEnd);
            } else if (hasComma(p + 1, blockEnd) || p->type != TokenType::TUPLE_OPEN) {
                dest.push_back(*p);
                simplifyBlockExpression(dest, p + 1, blockEnd);
                dest.push_back(*blockEnd);
            } else {
                simplifyBlockExpression(dest, p + 1, blockEnd);
            }
            p = blockEnd;
        } else {
            dest.push_back(*p);
        }
    }
}

void findCallSequenceAndPushPreviousTokens(std::vector<Token>& dest, const Token* begin, const Token* end, const Token*& callBegin) {
    for (const Token* p = begin; p != end; p++) {
        if (isOpenBracket(p->type)) {
            p = findBlockClose(p, end);
            continue;
        }
        if (p->type == TokenType::MOV || p->type == TokenType::SEND) {
            for (const Token* p1 = begin; p1 != p; p1++) {
                dest.push_back(*p1);
            }
            dest.push_back(*p);
            dest.push_back(p[1]);
            callBegin = p + 2;
            return;
        }
        if (p->type == TokenType::ASS) {
            for (const Token* p1 = begin; p1 != p; p1++) {
                dest.push_back(*p1);
            }
            dest.push_back(*p);
            callBegin = p + 1;
            return;
        }
        if (p->type == TokenType::MESSAGE_DECL) {
            for (const Token* p1 = begin; p1 != p; p1++) {
                dest.push_back(*p1);
            }
            while (p->type != TokenType::MESSAGE_OPEN) {
                dest.push_back(*p);
                p++;
            }
            callBegin = p;
            return;
        }
        if (p->type == TokenType::NEWLINE || isCloseBracket(p->type)) {
            callBegin = begin;
            return;
        }
    }
    callBegin = begin;
}

void fixTokens4(std::vector<Token>& dest, const Token* begin, const Token* end) {
    for (const Token* p = begin; p != end && p != (end + 1); p++) {
        const Token* callBegin;
        findCallSequenceAndPushPreviousTokens(dest, p, end, callBegin);
        if (callBegin == end) {
            return;
        }
        int count = countCallSequence(callBegin, end);
        for (int i = 0; i < count - 1; i++) {
            dest.push_back(Token(TokenType::CALL, ""));
        }
        p = callBegin;
        while (count != 0) {
            if (isOpenBracket(p->type)) {
                const Token* blockEnd = findBlockClose(p, end);
                if ((p + 1) == blockEnd) {
                    dest.push_back(*p);
                    dest.push_back(*blockEnd);
                } else if (hasComma(p + 1, blockEnd) || p->type != TokenType::TUPLE_OPEN) {
                    dest.push_back(*p);
                    fixTokens4(dest, p + 1, blockEnd);
                    dest.push_back(*blockEnd);
                } else {
                    fixTokens4(dest, p + 1, blockEnd);
                }
                p = blockEnd;
            } else if (p->type >= TokenType::BOOL && p->type <= TokenType::UPPER_NAME) {
                dest.push_back(*p);
            } else if (p->type == TokenType::NEWLINE && isCloseBracket(p->type)) {
                break;
            } else {
                throw TokenizerPanic("This error cannot be real in fixTokens4: " + tokenTypeToString(p[0].type));
            }
            p++;
            count--;
        }
        if (p != end) {
            dest.push_back(*p);
        }
    }
}

std::vector<Token> fixTokens(std::vector<Token> v) {
    std::vector<Token> tokens;
    fixTokens1(tokens, &*v.cbegin(), &*v.cend());
    v = {};
    fixTokens2(v, &*tokens.cbegin(), &*tokens.cend(), TokenType::CODE_OPEN);
    tokens = {};
    fixTokens3(tokens, &*v.cbegin(), &*v.cend(), TokenType::CODE_OPEN);
    v = {};
    fixTokens4(v, &*tokens.cbegin(), &*tokens.cend());
    return v;
}
