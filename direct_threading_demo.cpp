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
#include <limits>

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
using OpCode = void*;

constexpr size_t numberOfOpCodes = 9;
constexpr size_t OperatorToOpCodeIndex(const char& opcodeCharacter) {
    switch (opcodeCharacter) {
        case '+': return 0;
        case '-': return 1;
        case '<': return 2;
        case '>': return 3;
        case '[': return 4;
        case ']': return 5;
        case '.': return 6;
        case ',': return 7;
        case '\0': return 8;
        default: return std::numeric_limits::max<size_t>();
    }
}

//  The instruction stream is mainly OpCodes but there are some
//  integer arguments interspersed. Strictly speaking this makes this
//  interpreter a hybrid between direct/indirect threading.
union  Instruction {
    OpCode opcode;
    int operand;
};

//  This class is responsible for translating the stream of source code
//  into a vector<Instruction>. It is passed a mapping from characters
//  to the addresses-of-labels, so it can plant (aka append) the exact
//  pointer to the implementing code. 
class CodePlanter {
    std::ifstream input;                //  The source code to be read in.
    const std::array<numberOfOpCodes, OpCode> & opcode_map;
    std::vector<Instruction> & program; 
    std::vector<int> indexes;           //  Responsible for managing [ ... ] loops.

public:
    CodePlanter( 
        std::string_view filename, 
        const std::array<, OpCode> & opcode_map, 
        std::vector<Instruction> & program 
    ) :
        input( filename.data(), std::ios::in ),
        opcode_map( opcode_map ),
        program( program )
    {}
    CodePlanter(CodePlanter&&) noexcept = default;
    CodePlanter(const CodePlanter&) noexcept = default;
    // Contains references not not trival assignable
private:
    void plantChar( char ch ) {
        //  Guard - skip characters that do not correspond to abstract machine
        //  operations
        const auto opcodeIndex = OperatorToOpCodeIndex(ch);
        return_if( opcodeIndex > numberOfOpCodes );

        //  Body
        program.push_back( { opcode_map[opcodeIndex] } );
        //  If we are dealing with loops, we plant the absolute index of the
        //  operation in the program we want to jump to. This can be improved
        //  fairly easily.
        if ( ch == '[' ) {
            indexes.emplace_back( program.size() );
            program.emplace_back( {nullptr} );         //  Dummy value, will be overwritten.
        } else if ( ch == ']' ) {
            size_t end = program.size();
            int start = indexes.back();
            indexes.pop_back();
            program[ start ].operand = end + 1;     //  Overwrite the dummy value.
            // Note this is a C++ 20 feature:
            program.push_back( { .operand=( start + 1 ) } );
        }
    }

public:
    void plantProgram() {
        while (true) {
            char ch = input.get();
            break_unless( input.good() );
            plantChar( ch );
        }
        program.push_back( { opcode_map[OperatorToOpCodeIndex('\0')] } );
    }
};

using num = unsigned char;

class Engine {
    std::array<numberOfOpCodes, OpCode> opcode_map;
    std::vector<Instruction> program;
    std::vector<num> memory;
public:
    Engine() : 
        memory( 30000 )
    {}

public:
    void runFile( std::string_view filename, bool header_needed ) {
        if ( header_needed ) {
            std::cerr << "# Executing: " << filename << std::endl;
        }

        opcode_map = {
            { &&INCR },
            { &&DECR },
            { &&LEFT },
            { &&RIGHT },
            { &&OPEN },
            { &&CLOSE },
            { &&PUT },
            { &&GET },
            { &&HALT }
        };
        
        CodePlanter planter{ filename, opcode_map, program };
        planter.plantProgram();

        std::noskipws( std::cin );

        auto program_data = program.begin();
        auto loc = memory.begin();
        goto *(pc++->opcode);

        ////////////////////////////////////////////////////////////////////////
        //  Control flow does not reach this position! 
        //  Instructions are listed below.
        ////////////////////////////////////////////////////////////////////////
        
        // Give hints to the compiler
#        if defined(_MSC_VER) && !defined(__clang__) // MSVC
            __assume(false);
#        else // GCC, Clang
            __builtin_unreachable();
#        endif

#        define ON_LABEL_DO ( LABEL , code ) LABEL: if (DEBUG) std::cout << #LABEL << /*flush*/ std::endl; [&](){ code }(); goto *(pc++->opcode)
    ON_LABEL_DO(INCR, *loc += 1; );
    ON_LABEL_DO(DECR, *loc -= 1; );
    ON_LABEL_DO(RIGHT, std::next(loc));
    ON_LABEL_DO(LEFT, std::prev(loc));
    ON_LABEL_DO(PUT, std::cout << *loc; );
    ON_LABEL_DO(GET, 
            char ch;
            std::cin.get( ch );
            if (std::cin.good()) {
                *loc = ch;
            }
        ); 
    ON_LABEL_DO(OPEN,
            int n = pc++->operand;
            if ( *loc == 0 ) {
                pc = std::advance(program_data.begin(), n);
            }
        );
    ON_LABEL_DO(CLOSE,
            int n = pc++->operand;
            if ( *loc == 0 ) {
                pc = std::advance(program_data.begin(), n);
            }
        );
    HALT:
        if ( DEBUG ) std::cout << "DONE!" << std::endl;
        return;
    } // End function
}; // End class

/*
Each argument is the name of a Brainf*ck source file to be compiled into
threaded coded and executed.
*/
int main( int argc, char * argv[] ) {
    const std::vector<std::string_view> args(std::next(argv), std::advance(argv, argc));
    for (auto arg : args) {
        Engine engine;
        engine.runFile( arg, args.size() > 1 );
    }
    return 0;
}
