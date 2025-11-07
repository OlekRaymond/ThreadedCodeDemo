/*
This code demonstrates how to implement direct-threaded code using GNU C++ by
exploiting a little-known extension - the ability to take the address of a label,
store it in a pointer, and jump to it through the pointer.

In this example, we translate Brainf*ck into threaded code, where the program
is a sequence of abstract-machine instructions. Each instruction of the program
is a pointer to a label that represents the code to execute, optionally
followed by some integer data.

We use Brainf*ck because it has very few instructions, which makes the crucial 
method (Engine::runFile) fit on a single screen.
*/

#include <iostream>
#include <vector>
#include <fstream>
#include <map>
#include <sstream>
#include "unreachable.hpp"

namespace computed_gotos {

//  Use this to turn on or off some debug-level tracing.
constexpr bool DEBUG = false;

//  Syntactic sugar to emphasise the 'break'.
#define break_if( E ) if ( E ) break
#define break_unless( E ) if (!(E)) break

//  Syntactic sugar to emphasise the 'return'. Usage:
//      return_if( E );
//      return_if( E )( Result )
#define return_if( E ) if (E) return

//  We use the address of a label to play the role of an operation-code.
using OpCode = void *;

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
            program.push_back( {nullptr} );         //  Dummy value, will be overwritten.
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
    inline void runFile( std::string_view filename, bool header_needed, StreamType& outStream = std::cout ) {
        if ( header_needed ) {
            std::cerr << "# Executing: " << filename << std::endl;
        }

        opcode_map = {
            { '+', &&INCR },
            { '-', &&DECR },
            { '<', &&LEFT },
            { '>', &&RIGHT },
            { '[', &&OPEN },
            { ']', &&CLOSE },
            { '.', &&PUT },
            { ',', &&GET },
            { '\0', &&HALT }
        };
        
        CodePlanter planter( filename, opcode_map, program );
        planter.plantProgram();

        std::noskipws( std::cin );

        auto program_data = program.data();
        Instruction * pc = &program_data[0];
        num * loc = &memory.data()[0];
        goto *(pc++->opcode);

        ////////////////////////////////////////////////////////////////////////
        //  Control flow does not reach this position! 
        //  Instructions are listed below.
        ////////////////////////////////////////////////////////////////////////

    INCR:
        if ( DEBUG ) std::cout << "INCR" << std::endl;
        *loc += 1;
        goto *(pc++->opcode);
    DECR:
        if ( DEBUG ) std::cout << "DECR" << std::endl;
        *loc -= 1;
        goto *(pc++->opcode);
    RIGHT:
        if ( DEBUG ) std::cout << "RIGHT" << std::endl;
        loc += 1;
        goto *(pc++->opcode);
    LEFT:
        if ( DEBUG ) std::cout << "LEFT" << std::endl;
        loc -= 1;
        goto *(pc++->opcode);
    PUT:
        if ( DEBUG ) std::cout << "PUT" << std::endl;
        {
            num i = *loc;
            outStream << i;
        }
        goto *(pc++->opcode);
    GET:
        if ( DEBUG ) std::cout << "GET" << std::endl;
        {
            char ch;
            std::cin.get( ch );
            if (std::cin.good()) {
                *loc = ch;
            }
        }
        goto *(pc++->opcode);
    OPEN:
        if ( DEBUG ) std::cout << "OPEN" << std::endl;
        {
            int n = pc++->operand;
            if ( *loc == 0 ) {
                pc = &program_data[n];
            }
            goto *(pc++->opcode);
        }
    CLOSE:
        if ( DEBUG ) std::cout << "CLOSE" << std::endl;
        {
            int n = pc++->operand;
            if ( *loc != 0 ) {
                pc = &program_data[n];
            }
            goto *(pc++->opcode);
        }
    HALT:
        if ( DEBUG ) std::cout << "DONE!" << std::endl;
        return;
    } // End function

    template<typename StreamType = std::ostream>
    inline void runMacros( std::string_view filename, bool header_needed, StreamType& outStream = std::cout ) {
        if ( header_needed ) {
            outStream << "# Executing: " << filename << "\n";
        }

        opcode_map = {
            { '+', &&INCR },
            { '-', &&DECR },
            { '<', &&LEFT },
            { '>', &&RIGHT },
            { '[', &&OPEN },
            { ']', &&CLOSE },
            { '.', &&PUT },
            { ',', &&GET },
            { '\0', &&HALT }
        };
        
        CodePlanter planter( filename, opcode_map, program );
        planter.plantProgram();

        std::noskipws( std::cin );

        auto program_data = program.data();
        Instruction * pc = &program_data[0];
        num * loc = &memory.data()[0];
        goto *(pc++->opcode);

        ////////////////////////////////////////////////////////////////////////
        //  Control flow does not reach this position! 
        //  Instructions are listed below.
        ////////////////////////////////////////////////////////////////////////
#       define ON_LABEL_DO( LABEL , code ) LABEL : if constexpr (DEBUG) {outStream << #LABEL << "\n"; } code goto *(pc++->opcode)
    ON_LABEL_DO(INCR, *loc += 1; );
    ON_LABEL_DO(DECR, *loc -= 1; );
    ON_LABEL_DO(RIGHT, loc += 1; );
    ON_LABEL_DO(LEFT, loc -= 1; );
    ON_LABEL_DO(PUT,
            num i = *loc;
            outStream << i;
        );
    ON_LABEL_DO(GET,
            char ch;
            std::cin.get( ch );
            if (std::cin.good()) {
                *loc = ch;
            }
        );
    ON_LABEL_DO(OPEN, {
            int n = pc++->operand;
            if ( *loc == 0 ) {
                pc = &program_data[n];
            }
        }
    );
    ON_LABEL_DO(CLOSE, 
            int n = pc++->operand;
            if ( *loc != 0 ) {
                pc = &program_data[n];
            }
    );
    ON_LABEL_DO(HALT, return; ); // NEXT is never reached
    } // End function

    # undef ON_LABEL_DO
    template<typename StreamType = std::ostream>
    inline void runUnreachable( std::string_view filename, bool header_needed, StreamType& outStream = std::cout ) {
        if ( header_needed ) {
            outStream << "# Executing: " << filename << "\n";
        }

        opcode_map = {
            { '+', &&INCR },
            { '-', &&DECR },
            { '<', &&LEFT },
            { '>', &&RIGHT },
            { '[', &&OPEN },
            { ']', &&CLOSE },
            { '.', &&PUT },
            { ',', &&GET },
            { '\0', &&HALT }
        };
        
        CodePlanter planter( filename, opcode_map, program );
        planter.plantProgram();

        std::noskipws( std::cin );

        auto program_data = program.data();
        Instruction * pc = &program_data[0];
        num * loc = &memory.data()[0];
        goto *(pc++->opcode);

        ////////////////////////////////////////////////////////////////////////
        //  Control flow does not reach this position! 
        //  Instructions are listed below.
        ////////////////////////////////////////////////////////////////////////
        __builtin_unreachable();

        #define ON_LABEL_DO( LABEL , code ) LABEL : if constexpr (DEBUG) {outStream << #LABEL << "\n"; } code goto *(pc++->opcode); __builtin_unreachable();
        ON_LABEL_DO(INCR, *loc += 1; );
        ON_LABEL_DO(DECR, *loc -= 1; );
        ON_LABEL_DO(RIGHT, loc += 1; );
        ON_LABEL_DO(LEFT, loc -= 1; );
        ON_LABEL_DO(PUT,
                num i = *loc;
                outStream << i;
            );
        ON_LABEL_DO(GET,
                char ch;
                std::cin.get( ch );
                if (std::cin.good()) {
                    *loc = ch;
                }
            );
        ON_LABEL_DO(OPEN, {
                int n = pc++->operand;
                if ( *loc == 0 ) {
                    pc = &program_data[n];
                }
            }
        );
        ON_LABEL_DO(CLOSE,
                int n = pc++->operand;
                if ( *loc != 0 ) {
                    pc = &program_data[n];
                }
        );
        ON_LABEL_DO(HALT, return; ); // NEXT is never reached
    } // End function
    # undef ON_LABEL_DO

};

}
/*
Each argument is the name of a Brainf*ck source file to be compiled into
threaded coded and executed.
*/

#include <benchmark/benchmark.h>
#include <gtest/gtest.h>

namespace computed_gotos {
static void CG_Labels(benchmark::State& state) {
    Engine engine{};
    std::ostringstream output{};
    for (auto _ : state) {
        engine.runFile( "../sierpinski.bf", false, output );
    }
}
BENCHMARK(CG_Labels);

static void CG_LabelsMacros(benchmark::State& state) {
    Engine engine{};
    std::ostringstream output{};
    for (auto _ : state) {
        engine.runMacros( "../sierpinski.bf", false, output);
    }
}
BENCHMARK(CG_LabelsMacros);

static void CG_LabelsUnreachable(benchmark::State& state) {
    Engine engine{};
    std::ostringstream output{};
    for (auto _ : state) {
        engine.runUnreachable( "../sierpinski.bf", false, output );
    }
}
BENCHMARK(CG_LabelsUnreachable);

TEST( CG_DirectThreadedCode, NoChange ) {
    std::stringstream output_expected{};
    std::stringstream output_macros{};
    {
        // std::cout << "Running without lambdas" << std::endl;
        Engine engine{};
        engine.runFile( "../sierpinski.bf", false, output_expected );
        engine.runFile( "../hello.bf", false, output_expected );
        engine.runFile( "../head.bf", false, output_expected );

        // std::cout << "Ran without lambdas" << std::endl;
    }
    {
        // std::cout << "Running with lambdas" << std::endl;
        Engine engine{};
        engine.runMacros( "../sierpinski.bf", false, output_macros );
        engine.runMacros( "../hello.bf", false, output_macros );
        engine.runMacros( "../head.bf", false, output_macros );
        // std::cout << "Ran with lambdas" << std::endl;
    }
    ASSERT_TRUE( output_expected.good()     );
    ASSERT_TRUE( output_macros  .good()     );
    ASSERT_NE(   output_expected.tellp(), 0 );
    ASSERT_NE(   output_macros  .tellp(), 0 );


    ASSERT_EQ( output_expected.str(), output_macros.str() );
}

} // namespace two

int main(int argc, char** argv) {   
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    {
        ::testing::InitGoogleTest(&argc, argv);
        const auto test_result = RUN_ALL_TESTS();
        if (test_result != 0) {
            std::cout << "Did not run benchmarks due to test failures." << std::endl;
            return test_result;
        }
    }
    char arg0_default[] = "benchmark";
    char* args_default = arg0_default;
    if (!argv) {                      
        argc = 1;                       
        argv = &args_default;           
    }                                 
    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
}
