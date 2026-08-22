#pragma once
#include <sstream>
#include <fstream>
#include "vm.h"
#include "builtin/builtin.h"

std::unordered_map<std::string, std::pair<BuitinFun, uint32_t>> vm::createFuns() {
    return {
        // type
        {"INT", {INT, 1}},
        {"ROUND", {ROUND, 1}},
        {"FLOOR", {FLOOR, 1}},
        {"CEIL", {CEIL, 1}},
        {"FLOAT", {FLOAT, 1}},
        {"CHAR", {CHAR, 1}},
        {"STRING", {STRING, 1}},
        {"BYTES", {BYTES, 1}},
        {"UINT", {UINT, 1}},
        {"ARR", {ARR, 1}},
        {"TUPLE", {TUPLE, 1}},
        {"DICT", {DICT, 1}},
        {"ITERATE", {ITERATE, 2}},
        {"COMPLEX", {COMPLEX, 1}},
        {"MATRIX", {MATRIX, 1}},
        {"EVAL", {EVAL, 5}},
        {"ASYNC", {ASYNC, 2}},
        {"TYPE", {TYPE, 1}},
        {"CAST", {CAST, 1}},

        // collection
        {"GET", {GET, 2}},
        {"SIZE", {SIZE, 1}},
        {"DROP", {DROP, 2}},
        {"TAKE", {TAKE, 2}},
        {"FIND", {FIND, 2}},
        {"REDUCE", {REDUCE, 3}},

        // compare
        {"EQ", {EQ, 2}},
        {"NEQ", {NEQ, 2}},
        {"LT", {LT, 2}},
        {"LE", {LE, 2}},
        {"GT", {GT, 2}},
        {"GE", {GE, 2}},

        // bitwise
        {"NOT", {NOT, 1}},
        {"AND", {AND, 2}},
        {"OR", {OR, 2}},
        {"XOR", {XOR, 2}},
        {"SHL", {SHL, 2}},
        {"SHR", {SHR, 2}},

        // basic math
        {"ABS", {ABS, 1}},
        {"ADD", {ADD, 2}},
        {"NEG", {NEG, 1}},
        {"MUL", {MUL, 2}},
        {"INV", {INV, 1}},
        {"MOD", {MOD, 2}},
        {"POW", {POW, 2}},

        {"MULM", {MULM, 2}},
        {"INVM", {INVM, 1}},
        {"POWM", {POWM, 2}},

        // int math
        {"MOD_POW", {MOD_POW, 3}},
        {"BIT_MATRIX_MUL", {BIT_MATRIX_MUL, 2}},

        // float math
        {"SIN", {SIN, 1}},
        {"COS", {COS, 1}},
        {"TAN", {TAN, 1}},
        {"ASIN", {ASIN, 1}},
        {"ACOS", {ACOS, 1}},
        {"ATAN", {ATAN, 1}},
        {"SINH", {SINH, 1}},
        {"COSH", {COSH, 1}},
        {"TANH", {TANH, 1}},
        {"EXP", {EXP, 1}},
        {"LOG", {LOG, 1}},
        {"LOG2", {LOG2, 1}},
        {"LOG10", {LOG10, 1}},

        {"SINM", {SINM, 1}},
        {"COSM", {COSM, 1}},
        {"TANM", {TANM, 1}},
        {"ASINM", {ASINM, 1}},
        {"ACOSM", {ACOSM, 1}},
        {"ATANM", {ATANM, 1}},
        {"SINHM", {SINHM, 1}},
        {"COSHM", {COSHM, 1}},
        {"TANHM", {TANHM, 1}},
        {"EXPM", {EXPM, 1}},
        {"LOGM", {LOGM, 1}},
        {"LOG2M", {LOG2M, 1}},
        {"LOG10M", {LOG10M, 1}},

        // complex math
        {"ARG", {ARG, 1}},
        {"CONJ", {CONJ, 1}},
        {"NORM", {NORM, 1}},
        {"POLAR", {POLAR, 2}},

        // matrix math
        {"DET", {DET, 1}},
        {"TRANSPOSE", {TRANSPOSE, 1}},
        {"GET_ROW", {GET_ROW, 2}},
        {"GET_COL", {GET_COL, 2}},
    };
}

std::unordered_map<std::string, BuitinMessage> vm::createMessages() {
    return {
        {"print", print},
        {"handle_http", handle_http},
        {"send", send},
    };
}

VM build(std::string path, std::atomic<bool>& interrupt) {
    std::ifstream fin(path);
    if (!fin.is_open()) {
        throw std::invalid_argument("Could not open Funtalk source file");
    }
    std::stringstream buf;
    buf << fin.rdbuf();
    return VM(
        buf.str(),
        vm::messageObj,
        interrupt
    );
}
