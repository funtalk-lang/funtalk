#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <cstdint>
#include "split.h"

enum class TokenType {
    BOOL, INT, FLOAT, CHAR, STRING,
    LOWER_NAME, SNAKE_NAME, UPPER_NAME, NONLOCAL_MESSAGE, MESSAGE,
    CODE_OPEN, CODE_CLOSE,
    ARR_OPEN, ARR_CLOSE,
    TUPLE_OPEN, TUPLE_CLOSE,
    MESSAGE_OPEN, MESSAGE_CLOSE,
    ASS, DOT, CALL, SEND, NEWLINE, MOV, FUNC_DECL, MESSAGE_DECL
};

class TokenizerPanic : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class FunTalkTokenizationError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct Token {
    TokenType type;
    std::string value;
    Token(TokenType type, std::string value) : type(type), value(std::move(value)) {}
};

std::string tokenTypeToString(TokenType type);
std::string blockTypeToString(TokenType type);
void printToken(const Token& token);

TokenType determineIdentifierType(const std::string& str);
TokenType determineLiteralType(const std::string& str);

std::vector<Token> tokenize(const std::vector<std::string>& v);

bool isOpenBracket(TokenType type);
bool isCloseBracket(TokenType type);

const Token* findBlockClose(const Token* begin, const Token* end);
bool checkMessageDeclTuple(const Token* begin, const Token* end);
void addMessageDeclArgs(std::vector<Token>& dest, const Token* begin, const Token* end);

void fixTokens1(std::vector<Token>& dest, const Token* begin, const Token* end);
void fixTokens2(std::vector<Token>& dest, const Token* begin, const Token* end, TokenType blockType);
void fixTokens3(std::vector<Token>& dest, const Token* begin, const Token* end, TokenType blockType);

int countCallSequence(const Token* begin, const Token* end, const Token* callEnd);
bool hasComma(const Token* begin, const Token* end);
void simplifyBlockExpression(std::vector<Token>& dest, const Token* begin, const Token* end);
void findCallSequenceAndPushPreviousTokens(std::vector<Token>& dest, const Token* begin, const Token* end, const Token*& callBegin);
void fixTokens4(std::vector<Token>& dest, const Token* begin, const Token* end);

std::vector<Token> fixTokens(std::vector<Token> v);
