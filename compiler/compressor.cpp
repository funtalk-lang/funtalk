#include "compressor.h"

std::string to_string(CompressedTokenType type) {
    switch (type) {
        case CompressedTokenType::BOOL:             return "BOOL";
        case CompressedTokenType::INT:              return "INT";
        case CompressedTokenType::FLOAT:            return "FLOAT";
        case CompressedTokenType::CHAR:             return "CHAR";
        case CompressedTokenType::STRING:           return "STRING";
        case CompressedTokenType::NAME:             return "NAME";
        case CompressedTokenType::NONLOCAL_MESSAGE: return "NONLOCAL_MESSAGE";
        case CompressedTokenType::MESSAGE:          return "MESSAGE";
        case CompressedTokenType::CODE:             return "CODE";
        case CompressedTokenType::ARR:              return "ARR";
        case CompressedTokenType::TUPLE:            return "TUPLE";
        case CompressedTokenType::CALL:             return "CALL";
        case CompressedTokenType::SEND:             return "SEND";
        case CompressedTokenType::MOV:              return "MOV";
        case CompressedTokenType::FUNC_DECL:        return "FUNC_DECL";
        case CompressedTokenType::MESSAGE_DECL:     return "MESSAGE_DECL";
        case CompressedTokenType::BUILT_IN_FUN:     return "BUILT_IN_FUN";
        case CompressedTokenType::BUILT_IN_MESSAGE: return "BUILT_IN_MESSAGE";
        case CompressedTokenType::MESSAGE_END:      return "MESSAGE_END";
        default:                                    return "UNKNOWN CompressedTokenType";
    }
}

CompressedToken::CompressedToken(CompressedTokenType type, uint64_t value) : type(type), value(value) {}

bool operator==(CompressedToken a, CompressedToken b) {
    return a.type == b.type && a.value == b.value;
}

SymbolTable::SymbolTable() {}

std::string expand_standard_escapes(const std::string& input) {
    if (input.empty()) return "";
    std::regex escape_regex(R"(\\(.))");
    std::string result;
    result.reserve(input.length());
    auto words_begin = std::sregex_iterator(input.begin(), input.end(), escape_regex);
    auto words_end = std::sregex_iterator();
    size_t last_pos = 0;
    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::smatch match = *i;
        result.append(input, last_pos, match.position() - last_pos);
        char escape_char = match[1].str()[0];
        switch (escape_char) {
            case 'n':  result += '\n'; break;
            case 't':  result += '\t'; break;
            case 'r':  result += '\r'; break;
            case 'b':  result += '\b'; break;
            case 'f':  result += '\f'; break;
            case 'v':  result += '\v'; break;
            case '0':  result += '\0'; break;
            case '1':  result += '\1'; break;
            case '2':  result += '\2'; break;
            case '3':  result += '\3'; break;
            case '4':  result += '\4'; break;
            case '5':  result += '\5'; break;
            case '6':  result += '\6'; break;
            case '7':  result += '\7'; break;
            case '\\': result += '\\'; break;
            case '\"': result += '\"'; break;
            case '\'': result += '\''; break;
            default:
                result += match.str();
                break;
        }
        last_pos = match.position() + match.length();
    }
    result.append(input, last_pos, input.length() - last_pos);
    return result;
}

void SymbolTable::insert_from_tokens(const std::vector<Token>& tokens) {
    for (const auto& token : tokens) {
        switch (token.type) {
            case TokenType::STRING: {
                Bytes s = expand_standard_escapes(token.value.substr(1, token.value.size() - 2));
                if (string_to_id.find(s) == string_to_id.end()) {
                    id_to_string[size] = s;
                    string_to_id[id_to_string[size]] = size;
                    size++;
                }
                break;
            }
            case TokenType::LOWER_NAME:
            case TokenType::SNAKE_NAME:
            case TokenType::UPPER_NAME:
            case TokenType::NONLOCAL_MESSAGE: {
                if (string_to_id.find(token.value) == string_to_id.end()) {
                    id_to_string[size] = token.value;
                    string_to_id[id_to_string[size]] = size;
                    size++;
                }
                break;
            }
            case TokenType::MESSAGE: {
                size_t i = token.value.find(".");
                Bytes s = token.value.substr(0, i);
                if (string_to_id.find(s) == string_to_id.end()) {
                    id_to_string[size] = s;
                    string_to_id[id_to_string[size]] = size;
                    size++;
                }
                s = token.value.substr(i + 1);
                if (string_to_id.find(s) == string_to_id.end()) {
                    id_to_string[size] = s;
                    string_to_id[id_to_string[size]] = size;
                    size++;
                }
                break;
            }
            default:
                break;
        }
    }
}

uint64_t SymbolTable::push_back(const std::string& s) {
    id_to_string[size] = expand_standard_escapes(s);
    string_to_id[id_to_string[size]] = size;
    return size++;
}

char32_t decode_utf8_codepoint(const std::string& s, size_t& i) {
    if (i >= s.size()) return 0;
    unsigned char c = s[i];
    char32_t codepoint = 0;
    size_t extra_bytes = 0;
    if (c <= 0x7F) {
        codepoint = c;
        extra_bytes = 0;
    } else if ((c & 0xE0) == 0xC0) {
        codepoint = c & 0x1F;
        extra_bytes = 1;
    } else if ((c & 0xF0) == 0xE0) {
        codepoint = c & 0x0F;
        extra_bytes = 2;
    } else if ((c & 0xF8) == 0xF0) {
        codepoint = c & 0x07;
        extra_bytes = 3;
    } else {
        throw std::runtime_error("Invalid UTF-8 leading byte");
    }
    if (i + extra_bytes >= s.size()) {
        throw std::runtime_error("Incomplete UTF-8 sequence");
    }
    for (size_t j = 0; j < extra_bytes; ++j) {
        unsigned char next_byte = s[++i];
        if ((next_byte & 0xC0) != 0x80) {
            throw std::runtime_error("Invalid UTF-8 continuation byte");
        }
        codepoint = (codepoint << 6) | (next_byte & 0x3F);
    }
    return codepoint;
}

char32_t parse_char_literal(const std::string& literal) {
    std::string s = literal;
    if (s.size() >= 2 && s.front() == '\'' && s.back() == '\'') {
        s = s.substr(1, s.size() - 2);
    }
    if (s.empty()) {
        return 0;
    }
    std::string unescaped;
    unescaped.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\') {
            if (i + 1 >= s.size()) {
                throw std::runtime_error("Trailing backslash in literal");
            }
            i++;
            switch (s[i]) {
                case 'n':  unescaped.push_back('\n'); break;
                case 't':  unescaped.push_back('\t'); break;
                case 'r':  unescaped.push_back('\r'); break;
                case 'b':  unescaped.push_back('\b'); break;
                case 'f':  unescaped.push_back('\f'); break;
                case 'v':  unescaped.push_back('\v'); break;
                case '0':  unescaped.push_back('\0'); break;
                case '1':  unescaped.push_back('\1'); break;
                case '2':  unescaped.push_back('\2'); break;
                case '3':  unescaped.push_back('\3'); break;
                case '4':  unescaped.push_back('\4'); break;
                case '5':  unescaped.push_back('\5'); break;
                case '6':  unescaped.push_back('\6'); break;
                case '7':  unescaped.push_back('\7'); break;
                case '\\': unescaped.push_back('\\'); break;
                case '\'': unescaped.push_back('\''); break;
                case '\"': unescaped.push_back('\"'); break;
                case 'x': {
                    if (i + 2 >= s.size()) {
                        throw std::runtime_error("Invalid hex escape sequence");
                    }
                    std::string hex_str = s.substr(i + 1, 2);
                    auto val = std::stoul(hex_str, nullptr, 16);
                    unescaped.push_back(static_cast<char>(val));
                    i += 2;
                    break;
                }
                default:
                    throw std::runtime_error(std::string("Unknown escape sequence: \\") + s[i]);
            }
        } else {
            unescaped.push_back(s[i]);
        }
    }
    size_t index = 0;
    char32_t result = decode_utf8_codepoint(unescaped, index);
    if (index + 1 < unescaped.size()) {
        throw std::runtime_error("Character literal contains multiple characters");
    }
    return result;
}

void compress(std::vector<CompressedToken>& dest, const SymbolTable& symbolTable, const Token* begin, const Token* end) {
    dest.push_back(CompressedToken(CompressedTokenType::CODE, 0));
    for (const Token* p = begin; p < end; p++) {
        if (isOpenBracket(p->type)) {
            p = findBlockClose(p, end);
        } else if (p->type == TokenType::MOV || p->type == TokenType::FUNC_DECL || p->type == TokenType::SEND || p->type == TokenType::MESSAGE_DECL) {
            dest[0].value++;
        }
    }
    for (const Token* p = begin; p < end; p++) {
        switch (p->type) {
            case TokenType::BOOL:
                dest.push_back(CompressedToken(CompressedTokenType::BOOL, p->value == "TRUE"));
                break;
            case TokenType::INT:
                dest.push_back(CompressedToken(CompressedTokenType::INT, std::stoll(p->value)));
                break;
            case TokenType::FLOAT:
                dest.push_back(CompressedToken(CompressedTokenType::FLOAT, std::bit_cast<uint64_t, double>(std::stod(p->value))));
                break;
            case TokenType::CHAR: {
                dest.push_back(CompressedToken(CompressedTokenType::CHAR, parse_char_literal(p->value)));
                break;
            }
            case TokenType::STRING:
                if (p->value.size() == 2) {
                    dest.push_back(CompressedToken(CompressedTokenType::STRING, 0));
                    break;
                }
                dest.push_back(CompressedToken(CompressedTokenType::STRING, symbolTable.string_to_id.at(expand_standard_escapes(p->value.substr(1, p->value.size() - 2)))));
                break;
            case TokenType::LOWER_NAME:
            case TokenType::SNAKE_NAME:
            case TokenType::UPPER_NAME:
                dest.push_back(CompressedToken(CompressedTokenType::NAME, symbolTable.string_to_id.at(p->value)));
                break;
            case TokenType::NONLOCAL_MESSAGE:
                dest.push_back(CompressedToken(CompressedTokenType::NONLOCAL_MESSAGE, symbolTable.string_to_id.at(p->value)));
                break;
            case TokenType::MESSAGE: {
                size_t i = p->value.find(".");
                dest.push_back(CompressedToken(CompressedTokenType::MESSAGE, symbolTable.string_to_id.at(p->value.substr(0, i)) | (symbolTable.string_to_id.at(p->value.substr(i + 1)) << 32)));
                break;
            }
            case TokenType::CODE_OPEN: {
                size_t i = 0;
                const Token* blockEnd = findBlockClose(p, end);
                for (const Token* p1 = p + 1; p1 != blockEnd; p1++) {
                    if (isOpenBracket(p1->type)) {
                        p1 = findBlockClose(p1, end);
                    } else if (p1->type == TokenType::MOV || p1->type == TokenType::FUNC_DECL || p1->type == TokenType::MESSAGE_DECL) {
                        i++;
                    }
                }
                dest.push_back(CompressedToken(CompressedTokenType::CODE, i));
                break;
            }
            case TokenType::CODE_CLOSE:
                break;
            case TokenType::ARR_OPEN: {
                const Token* blockEnd = findBlockClose(p, end);
                uint64_t i = 0;
                for (const Token* p1 = p + 1; p1 != blockEnd;) {
                    if (isOpenBracket(p1->type)) {
                        p1 = findBlockClose(p1, end) + 1;
                    } else if (p1->type == TokenType::CALL) {
                        size_t j = 2;
                        uint64_t level = 0;
                        p1++;
                        while (j != 0) {
                            if (p1->type == TokenType::CALL) {
                                j++;
                                p1++;
                                level++;
                            } else if (p1->type >= TokenType::BOOL && p1->type <= TokenType::UPPER_NAME) {
                                j--;
                                p1++;
                            } else if (isOpenBracket(p1->type)) {
                                j--;
                                p1 = findBlockClose(p1, end) + 1;
                            } else {
                                throw CompressorPanic("This error cannot be real");
                            }
                        }
                    } else {
                        p1++;
                    }
                    if (p1->type != TokenType::NEWLINE) {
                        i++;
                    }
                }
                dest.push_back(CompressedToken(CompressedTokenType::ARR, i));
                break;
            }
            case TokenType::ARR_CLOSE:
                break;
            case TokenType::TUPLE_OPEN: {
                const Token* blockEnd = findBlockClose(p, end);
                uint64_t i = 0;
                for (const Token* p1 = p + 1; p1 != blockEnd;) {
                    if (isOpenBracket(p1->type)) {
                        p1 = findBlockClose(p1, end) + 1;
                    } else if (p1->type == TokenType::CALL) {
                        size_t j = 2;
                        uint64_t level = 0;
                        p1++;
                        while (j != 0) {
                            if (p1->type == TokenType::CALL) {
                                j++;
                                p1++;
                                level++;
                            } else if (p1->type >= TokenType::BOOL && p1->type <= TokenType::UPPER_NAME) {
                                j--;
                                p1++;
                            } else if (isOpenBracket(p1->type)) {
                                j--;
                                p1 = findBlockClose(p1, end) + 1;
                            } else {
                                throw CompressorPanic("This error cannot be real");
                            }
                        }
                    } else {
                        p1++;
                    }
                    if (p1->type != TokenType::NEWLINE) {
                        i++;
                    }
                }
                dest.push_back(CompressedToken(CompressedTokenType::TUPLE, i));
                break;
            }
            case TokenType::TUPLE_CLOSE:
                break;
            case TokenType::MESSAGE_OPEN:
                break;
            case TokenType::MESSAGE_CLOSE:
                dest.push_back(CompressedToken(CompressedTokenType::MESSAGE_END, 0));
                break;
            case TokenType::ASS:
                break;
            case TokenType::DOT:
                break;
            case TokenType::CALL: {
                size_t i = 2;
                const Token* p1 = p + 1;
                while (i != 0) {
                    if (p1 >= end) {
                        throw CompressorPanic("Unexpected end of tokens in CALL sequence");
                    }
                    if (p1->type == TokenType::CALL) {
                        i++;
                        p1++;
                    } else if (p1->type >= TokenType::BOOL && p1->type <= TokenType::UPPER_NAME) {
                        i--;
                        p1++;
                    } else if (isOpenBracket(p1->type)) {
                        i--;
                        p1 = findBlockClose(p1, end) + 1;
                        if (p1 >= end) {
                            throw CompressorPanic("Unclosed bracket inside CALL");
                        }
                    } else {
                        throw CompressorPanic("This error cannot be real");
                    }
                }
                dest.push_back(CompressedToken(CompressedTokenType::CALL, 0));
                break;
            }
            case TokenType::SEND:
                dest.push_back(CompressedToken(CompressedTokenType::SEND, 0));
                break;
            case TokenType::NEWLINE:
                break;
            case TokenType::MOV:
                dest.push_back(CompressedToken(CompressedTokenType::MOV, 0));
                break;
            case TokenType::FUNC_DECL: {
                size_t i = 0;
                const Token* p1 = p + 1;
                while (p1 < end && p1->type != TokenType::ASS) {
                    i++;
                    p1++;
                }
                if (p1 >= end) {
                    throw CompressorPanic("Missing assignment operator after function declaration");
                }
                uint64_t j = 0;
                size_t level = 0;
                p1++;
                while (p1->type != TokenType::NEWLINE || level != 0) {
                    if (isOpenBracket(p1->type)) {
                        level++;
                        if (p1->type != TokenType::MESSAGE_OPEN) {
                            j++;
                        }
                    } else if (isCloseBracket(p1->type)) {
                        level--;
                    } else if (p1->type != TokenType::NEWLINE) {
                        j++;
                    }
                    p1++;
                }
                dest.push_back(CompressedToken(CompressedTokenType::FUNC_DECL, i | (j << 32)));
                break;
            }
            case TokenType::MESSAGE_DECL: {
                uint16_t i = 0;
                const Token* p1 = p + 1;
                while (p1->type != TokenType::MESSAGE_OPEN) {
                    i++;
                    p1++;
                }
                const Token* blockEnd = findBlockClose(p1, end);
                uint32_t j = 0;
                p1++;
                while (p1 != blockEnd) {
                    if (isOpenBracket(p1->type)) {
                        p1 = findBlockClose(p1, end);
                    } else if (p1->type == TokenType::MOV || p1->type == TokenType::FUNC_DECL || p1->type == TokenType::SEND || p1->type == TokenType::MESSAGE_DECL) {
                        j++;
                    }
                    p1++;
                }
                dest.push_back(CompressedToken(CompressedTokenType::MESSAGE_DECL, i | (uint64_t(j) << 16)));
                break;
            }
        }
    }
    std::vector<CompressedToken> dest1;
    for (const CompressedToken* p = &*dest.cbegin(); p < &*dest.cend(); p++) {
        if (p->type == CompressedTokenType::MESSAGE_DECL) {
            uint16_t i = 0;
            const CompressedToken* p1 = p;
            uint32_t j = 0;
            uint32_t count = 0;
            do {
                if (p1->type == CompressedTokenType::MESSAGE_DECL) {
                    j++;
                    count++;
                } else if (p1->type == CompressedTokenType::MESSAGE_END) {
                    j--;
                }
                p1++;
                i++;
            } while (j != 0);
            i -= count;
            dest1.push_back(CompressedToken(CompressedTokenType::MESSAGE_DECL, p->value | (uint64_t(i) << 48)));
        } else if (p->type != CompressedTokenType::MESSAGE_END) {
            dest1.push_back(*p);
        }
    }
    dest = std::move(dest1);
}

void printCompressedToken(const CompressedToken& t) {
    if (t.type == CompressedTokenType::MESSAGE_DECL) {
        std::cout << to_string(t.type) << ' ' << (t.value & 0xFFFF) << ' ' << ((t.value >> 16) & 0xFFFFFFFF) << ' ' << (t.value >> 48) << '\n';
    } else if (t.value > 0xFFFFFFFF) {
        std::cout << to_string(t.type) << ' ' << (t.value & 0xFFFFFFFF) << ' ' << (t.value >> 32) << '\n';
    } else {
        std::cout << to_string(t.type) << ' ' << t.value << '\n';
    }
}
