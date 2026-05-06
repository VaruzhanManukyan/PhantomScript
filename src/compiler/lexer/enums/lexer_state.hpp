#pragma once

enum class LexerState {
    START,

    IN_NUMBER,
    IN_IDENTIFIER,
    IN_STRING,
    IN_STRING_ESCAPE,

    IN_COMMENT_SINGLE,
    IN_COMMENT_MULTI,

    IN_OPERATOR,
};