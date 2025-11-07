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

namespace two {

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
    void runFile( std::string_view filename, bool header_needed, StreamType& outStream = std::cout ) {
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
    }

    template<typename StreamType = std::ostream>
    void runFileWithUnreachable( std::string_view filename, bool header_needed, StreamType& outStream = std::cout ) {
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
        NM_Unreachable

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
    }

    template<typename StreamType = std::ostream>
    void runFileLambdas( ::std::string_view filename, bool header_needed, StreamType& outStream = std::cout ) {
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
        num * loc = memory.data();
        goto *(pc++->opcode);

        ////////////////////////////////////////////////////////////////////////
        //  Control flow does not reach this position! 
        //  Instructions are listed below.
        ////////////////////////////////////////////////////////////////////////

    INCR:
        [&](){ 
            if ( DEBUG ) std::cout << "INCR" << std::endl;
                *loc += 1;
            }();
            goto *(pc++->opcode);
    DECR:
        [&](){ 

        if ( DEBUG ) std::cout << "DECR" << std::endl;
        *loc -= 1;
    }();
        goto *(pc++->opcode);

    RIGHT:
        [&](){ 

        if ( DEBUG ) std::cout << "RIGHT" << std::endl;
        loc += 1;
    }();
        goto *(pc++->opcode);

    LEFT:
        [&](){ 

        if ( DEBUG ) std::cout << "LEFT" << std::endl;
        loc -= 1;
    }();
        goto *(pc++->opcode);
        
    PUT:
        [&](){ 
        if ( DEBUG ) { std::cout << "PUT" << std::endl; }
        {
            num i = *loc;
            outStream << i;
        }
    }();
        goto *(pc++->opcode);
    GET:
        [&](){ 

        if ( DEBUG ) { std::cout << "GET" << std::endl; }
        {
            char ch;
            std::cin.get( ch );
            if (std::cin.good()) {
                *loc = ch;
            }
        }
    }();
        goto *(pc++->opcode);
    OPEN:
        [&](){ 

        if ( DEBUG ) { std::cout << "OPEN" << std::endl; }
        {
            int n = pc++->operand;
            if ( *loc == 0 ) {
                pc = &program_data[n];
            }
        }
        }();
            goto *(pc++->opcode);
    CLOSE:
        [&](){ 

        if ( DEBUG ) std::cout << "CLOSE" << std::endl;
        {
            int n = pc++->operand;
            if ( *loc != 0 ) {
                pc = &program_data[n];
            }
        }
        }();
            goto *(pc++->opcode);
    HALT:
        return [&](){
            if ( DEBUG ) std::cout << "DONE!" << std::endl;
            return;
        }();

    }

    template<typename StreamType = std::ostream>
    void runFileLambdasAndUnreachable( ::std::string_view filename, bool header_needed, StreamType& outStream = std::cout ) {
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
        NM_Unreachable
    INCR:
        [&](){ 
            if ( DEBUG ) std::cout << "INCR" << std::endl;
                *loc += 1;
            }();
            goto *(pc++->opcode);
    DECR:
        [&](){ 

        if ( DEBUG ) std::cout << "DECR" << std::endl;
        *loc -= 1;
    }();
        goto *(pc++->opcode);

    RIGHT:
        [&](){ 

        if ( DEBUG ) std::cout << "RIGHT" << std::endl;
        loc += 1;
    }();
        goto *(pc++->opcode);

    LEFT:
        [&](){ 

        if ( DEBUG ) std::cout << "LEFT" << std::endl;
        loc -= 1;
    }();
        goto *(pc++->opcode);
        
    PUT:
        [&](){ 

        if ( DEBUG ) std::cout << "PUT" << std::endl;
        {
            num i = *loc;
            outStream << i;
        }
    }();
        goto *(pc++->opcode);
    GET:
        [&](){ 

        if ( DEBUG ) std::cout << "GET" << std::endl;
        {
            char ch;
            std::cin.get( ch );
            if (std::cin.good()) {
                *loc = ch;
            }
        }
    }();
        goto *(pc++->opcode);
    OPEN:
        [&](){ 

        if ( DEBUG ) std::cout << "OPEN" << std::endl;
        {
            int n = pc++->operand;
            if ( *loc == 0 ) {
                pc = &program_data[n];
            }
        }
        }();
            goto *(pc++->opcode);
    CLOSE:
        [&](){ 

        if ( DEBUG ) std::cout << "CLOSE" << std::endl;
        {
            int n = pc++->operand;
            if ( *loc != 0 ) {
                pc = &program_data[n];
            }
        }
        }();
            goto *(pc++->opcode);
    HALT:
        return [&](){ 

        if ( DEBUG ) std::cout << "DONE!" << std::endl;
        return;
        }();

    }


#if defined(RequiresAChangeToThePlanterClass)
    template<typename StreamType = std::ostream>
    void runFileTailCallRecursion(::std::string_view filename, bool header_needed, StreamType& outStream = std::cout ) {
        if ( header_needed ) {
            std::cerr << "# Executing: " << filename << std::endl;
        }
        constexpr char INCR  = 0;
        constexpr char DECR  = 1;
        constexpr char LEFT  = 2;
        constexpr char RIGHT = 3;
        constexpr char OPEN  = 4;
        constexpr char CLOSE = 5;
        constexpr char PUT   = 6;
        constexpr char GET   = 7;
        constexpr char HALT  = 8;
        
        const auto tailCallable = [&](const char& opcode, auto& loc, auto& pc) {
            switch (OperatorToOpCodeIndex(opcode)) {
#define on_case( LABEL , code ) case LABEL: return [&](){ if constexpr (DEBUG) { std::cout << #LABEL << /*flush*/ std::endl; } code ; return tailCallable() }()
            on_case(INCR, *loc += 1; );
            on_case(DECR, *loc -= 1; );
            on_case(RIGHT, loc = std::next(loc);  );
            on_case(LEFT, loc = std::prev(loc);   );
            on_case(PUT, std::cout << *loc; );
            on_case(GET, char ch; std::cin.get( ch );
                if (std::cin.good()) { *loc = ch; }
            ); 
            on_case(OPEN,
                int n = pc++->operand;
                if ( *loc == 0 ) {
                    pc = &(program[n]);
                }
            );
            on_case(CLOSE,
                int n = pc++->operand;
                if ( *loc == 0 ) { pc = &(program[n]); }
            );

            case HALT:
                if constexpr ( DEBUG ) std::cout << "DONE!" << std::endl;
                return;
            } // End switch
    }; // EndLambda
    
    for ()

    } // End function
#endif
};

}
/*
Each argument is the name of a Brainf*ck source file to be compiled into
threaded coded and executed.
*/

#include <benchmark/benchmark.h>
#include <gtest/gtest.h>

namespace two {
static void Labels(benchmark::State& state) {
    Engine engine{};
    std::ostringstream output{};
    for (auto _ : state) {
        engine.runFile( "../sierpinski.bf", false, output );
    }
}
BENCHMARK(Labels);

static void LabelsLambdas(benchmark::State& state) {
    Engine engine{};
    std::ostringstream output{};
    for (auto _ : state) {
        engine.runFileLambdas( "../sierpinski.bf", false, output);
    }
}
BENCHMARK(LabelsLambdas);

static void LablesWithUnreachable(benchmark::State& state) {
    Engine engine{};
    std::ostringstream output{};
    for (auto _ : state) {
        engine.runFileWithUnreachable( "../sierpinski.bf", false, output );
    }
}
BENCHMARK(LablesWithUnreachable);

static void LablesWithLambdasAndUnreachable(benchmark::State& state) {
    Engine engine{};
    std::ostringstream output{};
    for (auto _ : state) {
        engine.runFileLambdasAndUnreachable( "../sierpinski.bf", false, output );
    }
}
BENCHMARK(LablesWithLambdasAndUnreachable);

TEST( DirectThreadedCode, NoChange ) {
    std::stringstream output_NoLambdas{};
    std::stringstream output_Lambdas{};
    {
        std::cout << "Running without lambdas" << std::endl;
        Engine engine{};
        engine.runFile( "../sierpinski.bf", false, output_NoLambdas );
        std::cout << "Ran without lambdas" << std::endl;
    }
    {
        std::cout << "Running with lambdas" << std::endl;
        Engine engine{};
        engine.runFileLambdas( "../sierpinski.bf", false, output_Lambdas );
        std::cout << "Ran with lambdas" << std::endl;
    }
    ASSERT_TRUE( output_NoLambdas.good()     );
    ASSERT_TRUE( output_Lambdas  .good()     );
    ASSERT_NE(   output_NoLambdas.tellp(), 0 );
    ASSERT_NE(   output_Lambdas  .tellp(), 0 );


    ASSERT_EQ( output_NoLambdas.str(), output_Lambdas.str() );
}

} // namespace two

