#pragma once

#include "common.hpp"

struct ArgParserException: public std::runtime_error {
    ArgParserException(std::string const& msg): std::runtime_error(msg) {}
};

template <typename T, typename U>
constexpr void validateType() {
#ifdef DEBUG
    if (typeid(T) != typeid(U)) {
        std::throw_with_nested(ArgParserException(
            std::format("incorrect type requested, expected: {0}, got: {1}", typeid(T).name(), typeid(U).name())
        ));
    }
#endif
}

class ArgParser {
private:
    wl_argument* rawArgs;
    const char* targetTyping;
    int typingCounter;

    void checkBounds() {
        if (this->typingCounter < 0) {
            std::throw_with_nested(ArgParserException("no more parameters, reached end of args list"));
        }
    }
public:
    ArgParser(wl_argument* rawArgs, const char* targetTyping) : rawArgs(rawArgs), targetTyping(targetTyping) {
        typingCounter = strlen(targetTyping) - 1;
    }

    void skip() {
        this->checkBounds();
        this->targetTyping++;
        this->typingCounter--;
        this->rawArgs++;
    }

    template<typename T>
    T next() {
        this->checkBounds();
        auto targetType = this->targetTyping[typingCounter];
        bool isNullable = false;
        if (targetType == '?') {
            this->typingCounter--;
            targetType = this->targetTyping[++typingCounter];
            isNullable = true;
        }

        // TODO: handle nullable (?) args
        T retVal;
        switch (targetType) {
        case 'i':
            validateType<T, int32_t>();
            retVal = reinterpret_cast<T>(rawArgs->i);
            break;
        case 'u':
            validateType<T, uint32_t>();
            retVal = reinterpret_cast<T>(rawArgs->u);
            break;
        case 'f':
            validateType<T, wl_fixed_t>();
            retVal = reinterpret_cast<T>(rawArgs->f);
            break;
        case 's':
            validateType<T, const char*>();
            retVal = reinterpret_cast<T>(rawArgs->s);
            break;
        case 'o':
            validateType<T, wl_object*>();
            retVal = reinterpret_cast<T>(rawArgs->o);
            break;
        case 'n':
            validateType<T, uint32_t>();
            retVal = reinterpret_cast<T>(rawArgs->n);
            break;
        case 'a':
            validateType<T, wl_array*>();
            retVal = reinterpret_cast<T>(rawArgs->a);
            break;
        case 'h':
            validateType<T, int32_t>();
            retVal = reinterpret_cast<T>(rawArgs->h);
            break;
        }
        // T retVal = *reinterpret_cast<T*>(this->rawArgs);
        this->skip();
        return retVal;
    }
};
