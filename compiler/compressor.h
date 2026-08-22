#pragma once
#include <regex>
#include <string>
#include <cstdint>
#include <vector>
#include <array>
#include <stdexcept>
#include <cuchar>
#include <climits>
#include <cstring>
#include "tokenizer.h"

constexpr size_t MAX_BYTES_SIZE = 8192;

class CallPanic : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

#include "../types/bytes.h"

struct SymbolTable;

enum class CompressedTokenType : uint64_t {
    BOOL, INT, FLOAT, CHAR, STRING,
    NAME,
    NONLOCAL_MESSAGE,
    MESSAGE,
    CODE,
    ARR,
    TUPLE,
    CALL, SEND, MOV, FUNC_DECL, MESSAGE_DECL,
    BUILT_IN_FUN, BUILT_IN_MESSAGE,
    MESSAGE_END,
};

std::string to_string(CompressedTokenType type);

struct CompressedToken {
    CompressedTokenType type;
    uint64_t value;
    CompressedToken() {};
    CompressedToken(CompressedTokenType type, uint64_t value);
};

bool operator==(CompressedToken a, CompressedToken b);

struct SymbolTable {
    std::unordered_map<BytesView, uint64_t> string_to_id;
    std::array<Bytes, 65536> id_to_string;
    uint64_t size = 0;
    SymbolTable();
    void insert_from_tokens(const std::vector<Token>& tokens);
    uint64_t push_back(const std::string& s);
};

class CompressorPanic : public std::runtime_error {
    using std::runtime_error::runtime_error;
};
void compress(std::vector<CompressedToken>& dest, const SymbolTable& symbolTable, const Token* begin, const Token* end);
void printCompressedToken(const CompressedToken& t);
