
#include <iostream>
#include <vector>
#include <fstream>
#include <map>
#include <sstream>
#include "unreachable.hpp"

namespace switch_ {

//  Use this to turn on or off some debug-level tracing.
constexpr bool DEBUG = false;

//  Syntactic sugar to emphasise the 'break'.
#define break_if( E ) if ( E ) break
#define break_unless( E ) if (!(E)) break

//  Syntactic sugar to emphasise the 'return'. Usage:
//      return_if( E );
//      return_if( E )( Result )
#define return_if( E ) if (E) return

enum struct OpCode {
    INCR,
    DECR,
    LEFT,
    RIGHT,
    OPEN,
    CLOSE,
    PUT,
    GET,
    HALT
};

//  The instruction stream is mainly OpCodes but there are some
//  integer arguments interspersed. Strictly speaking this makes this
//  interpreter a hybrid between direct/indirect threading.
union Instruction {
    OpCode opcode;
    int operand;
};

//  This class is responsible for translating the stream of source code
//  into a vector<Instruction>. It is passed a mapping from characters
//  to the addresses-of-labels, so it can plant (aka append) the exact
//  pointer to the implementing code. 
class CodePlanter {
    std::ifstream input;                //  The source code to be read in.
    const std::map<char, OpCode> & opcode_map;
    std::vector<Instruction> & program; 
    std::vector<int> indexes;           //  Responsible for managing [ ... ] loops.

public:
    CodePlanter( 
        std::string_view filename, 
        const std::map<char, OpCode> & opcode_map, 
        std::vector<Instruction> & program 
    ) :
        input( filename.data(), std::ios::in ),
        opcode_map( opcode_map ),
        program( program )
    {}

private:
    void plantChar( char ch ) {
        //  Guard - skip characters that do not correspond to abstract machine
        //  operations.
        auto it = opcode_map.find(ch);
        return_if( it == opcode_map.end() );

        //  Body
        program.push_back( { it->second } );
        //  If we are dealing with loops, we plant the absolute index of the
        //  operation in the program we want to jump to. This can be improved
        //  fairly easily.
        if ( ch == '[' ) {
            indexes.push_back( program.size() );
            program.push_back( {OpCode::HALT} );         //  Dummy value, will be overwritten.
        } else if ( ch == ']' ) {
            int end = program.size();
            int start = indexes.back();
            indexes.pop_back();
            program[ start ].operand = end + 1;     //  Overwrite the dummy value.
            program.push_back( { .operand=( start + 1 ) } );
        }
    }

public:
    void plantProgram() {
        for (;;) {
            char ch = input.get();
            break_unless( input.good() );
            plantChar( ch );
        }
        program.push_back( { opcode_map.at('\0') } );
    }
};

typedef unsigned char num;

class Engine {
    std::map<char, OpCode> opcode_map;
    std::vector<Instruction> program;
    std::vector<num> memory;
public:
    Engine() : 
        memory( 30000 )
    {}

public:
    template<typename StreamType = std::ostream>
    inline void runMacros( std::string_view filename, bool header_needed, StreamType& outStream = std::cout ) {
        if ( header_needed ) {
            outStream << "# Executing: " << filename << "\n";
        }
        

        opcode_map = {
            { '+',  OpCode::INCR },
            { '-',  OpCode::DECR },
            { '<',  OpCode::LEFT },
            { '>',  OpCode::RIGHT },
            { '[',  OpCode::OPEN },
            { ']',  OpCode::CLOSE },
            { '.',  OpCode::PUT },
            { ',',  OpCode::GET },
            { '\0', OpCode::HALT }
        };
        
        CodePlanter planter( filename, opcode_map, program );
        planter.plantProgram();

        std::noskipws( std::cin );

        auto program_data = program.data();
        Instruction * pc = &program_data[0];
        num * loc = &memory.data()[0];

        ////////////////////////////////////////////////////////////////////////
        //  Control flow does not reach this position! 
        //  Instructions are listed below.
        ////////////////////////////////////////////////////////////////////////
    #define ON_LABEL_DO( LABEL , code ) case OpCode::LABEL : if constexpr (DEBUG) {outStream << #LABEL << "\n"; } code break;
    while (true) { switch ( (*pc++).opcode) {
    ON_LABEL_DO(INCR, *loc += 1; );
    ON_LABEL_DO(DECR, *loc -= 1; );
    ON_LABEL_DO(RIGHT, loc += 1; );
    ON_LABEL_DO(LEFT, loc -= 1; );
    ON_LABEL_DO(PUT, {
            num i = *loc;
            outStream << i;
    }
        );
    ON_LABEL_DO(GET, {
            char ch;
            std::cin.get( ch );
            if (std::cin.good()) {
                *loc = ch;
            }
        }
        );
    ON_LABEL_DO(OPEN, {
            int n = pc++->operand;
            if ( *loc == 0 ) {
                pc = &program_data[n];
            }
        }
    );
    ON_LABEL_DO(CLOSE, {
            int n = pc++->operand;
            if ( *loc != 0 ) {
                pc = &program_data[n];
            }
        }
    );
    ON_LABEL_DO(HALT, return; ); // NEXT is never reached
    } // End switch
    } // End inf loop 
    } // End function

    template<typename StreamType = std::ostream>
    inline void runUnreachable( std::string_view filename, bool header_needed, StreamType& outStream = std::cout ) {
        if ( header_needed ) {
            outStream << "# Executing: " << filename << "\n";
        }
        

        opcode_map = {
            { '+',  OpCode::INCR },
            { '-',  OpCode::DECR },
            { '<',  OpCode::LEFT },
            { '>',  OpCode::RIGHT },
            { '[',  OpCode::OPEN },
            { ']',  OpCode::CLOSE },
            { '.',  OpCode::PUT },
            { ',',  OpCode::GET },
            { '\0', OpCode::HALT }
        };
        
        CodePlanter planter( filename, opcode_map, program );
        planter.plantProgram();

        std::noskipws( std::cin );

        auto program_data = program.data();
        Instruction * pc = &program_data[0];
        num * loc = &memory.data()[0];

        ////////////////////////////////////////////////////////////////////////
        //  Control flow does not reach this position! 
        //  Instructions are listed below.
        ////////////////////////////////////////////////////////////////////////
    #define ON_LABEL_DO( LABEL , code ) case OpCode:: LABEL : if constexpr (DEBUG) {outStream << #LABEL << "\n"; } code break;
    while (true) { switch ((*pc++).opcode) {
    ON_LABEL_DO(INCR, *loc += 1; );
    ON_LABEL_DO(DECR, *loc -= 1; );
    ON_LABEL_DO(RIGHT, loc += 1; );
    ON_LABEL_DO(LEFT, loc -= 1; );
    ON_LABEL_DO(PUT, {
            num i = *loc;
            outStream << i; }
        );
    ON_LABEL_DO(GET, {
            char ch;
            std::cin.get( ch );
            if (std::cin.good()) {
                *loc = ch;
            }
        }
        );
    ON_LABEL_DO(OPEN, {
            int n = pc++->operand;
            if ( *loc == 0 ) {
                pc = &program_data[n];
            }
        }
    );
    ON_LABEL_DO(CLOSE, {
            int n = pc++->operand;
            if ( *loc != 0 ) {
                pc = &program_data[n];
            }
        }
    );
    ON_LABEL_DO(HALT, return; ); // NEXT is never reached
    default: __builtin_unreachable();
    } // End switch
    } // End inf loop 
    } // End function

};

} // End namespace


#include <benchmark/benchmark.h>
#include <gtest/gtest.h>

namespace switch_ {
static void Switch(benchmark::State& state) {
    Engine engine{};
    std::ostringstream output{};
    for (auto _ : state) {
        engine.runMacros( "../sierpinski.bf", false, output);
    }
}
BENCHMARK(Switch);

static void SwitchUnreachable(benchmark::State& state) {
    Engine engine{};
    std::ostringstream output{};
    for (auto _ : state) {
        engine.runUnreachable( "../sierpinski.bf", false, output );
    }
}
BENCHMARK(SwitchUnreachable);
}