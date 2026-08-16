//Filename: main.cpp
//Assignment: Project 1: RISC-V Instruction Decoding
//Name: Niño De Mesa
//Grouped with Peter Estacio and Osh Ong
// Section: ENGG 123.01 - J1

#include <iostream>
#include <sstream>
#include <string>
#include <cstdint>
#include <cctype>
#include <iomanip>

using namespace std;

// ---------------------------------------------------------------------
// Bit-field extraction helpers
// ---------------------------------------------------------------------

// Extract bits [hi:lo] (inclusive) from a 32-bit word, right-justified.
static uint32_t bits(uint32_t word, int hi, int lo) {
    uint32_t mask = (hi - lo + 1 == 32) ? 0xFFFFFFFFu : ((1u << (hi - lo + 1)) - 1u);
    return (word >> lo) & mask;
}

// Sign-extend a value that occupies 'width' bits into a 32-bit signed int.
static int32_t signExtend(uint32_t value, int width) {
    uint32_t signBit = 1u << (width - 1);
    return static_cast<int32_t>((value ^ signBit) - signBit);
}

static string reg(uint32_t r) {
    return "x" + to_string(r);
}

// ---------------------------------------------------------------------
// Field container for one instruction word
// ---------------------------------------------------------------------
struct Fields {
    uint32_t word;
    uint32_t opcode, rd, funct3, rs1, rs2, funct7;

    explicit Fields(uint32_t w) : word(w) {
        opcode  = bits(w, 6, 0);
        rd      = bits(w, 11, 7);
        funct3  = bits(w, 14, 12);
        rs1     = bits(w, 19, 15);
        rs2     = bits(w, 24, 20);
        funct7  = bits(w, 31, 25);
    }

    int32_t immI() const {
        return signExtend(bits(word, 31, 20), 12);
    }
    int32_t immS() const {
        uint32_t imm = (bits(word, 31, 25) << 5) | bits(word, 11, 7);
        return signExtend(imm, 12);
    }
    int32_t immB() const {
        uint32_t imm = (bits(word, 31, 31) << 12) | (bits(word, 7, 7) << 11) |
                       (bits(word, 30, 25) << 5)  | (bits(word, 11, 8) << 1);
        return signExtend(imm, 13);
    }
    int32_t immU() const {
        // Upper 20 bits, already positioned; lower 12 bits are zero.
        return static_cast<int32_t>(word & 0xFFFFF000u);
    }
    int32_t immJ() const {
        uint32_t imm = (bits(word, 31, 31) << 20) | (bits(word, 19, 12) << 12) |
                       (bits(word, 20, 20) << 11) | (bits(word, 30, 21) << 1);
        return signExtend(imm, 21);
    }
};

// Prints the "cannot decode" fallback line.
static void printUndecodable(const Fields& f) {
    cout << "opcode = 0x" << hex << setw(2) << setfill('0') << f.opcode
         << ", funct7 = 0x" << setw(2) << setfill('0') << f.funct7
         << ", funct3 = 0x" << setw(1) << setfill('0') << f.funct3
         << dec << setfill(' ') << endl;
}

// Prints a note if the instruction's destination register is x0 for an
// instruction type where writing rd is meaningful (x0 is hardwired to 0,
// so such an encoding can never actually change its value -- flagged here
// as a semantically invalid/no-op-on-x0 case).
static void checkX0Write(uint32_t rd, const string& mnemonic) {
    if (rd == 0) {
        cout << "  [Note: destination register is x0 -- write has no effect; "
             << "x0 is hardwired to 0 (instruction: " << mnemonic << ")]" << endl;
    }
}

// ---------------------------------------------------------------------
// Main decode routine. Returns true if the instruction was decoded.
// ---------------------------------------------------------------------
static bool decodeInstruction(const Fields& f) {
    ostringstream asmOut;

    switch (f.opcode) {

    // ---------------- U-type ----------------
    case 0x37: { // LUI
        asmOut << "lui " << reg(f.rd) << ", " << (f.immU() >> 12);
        cout << asmOut.str() << endl;
        checkX0Write(f.rd, "lui");
        return true;
    }
    case 0x17: { // AUIPC
        asmOut << "auipc " << reg(f.rd) << ", " << (f.immU() >> 12);
        cout << asmOut.str() << endl;
        checkX0Write(f.rd, "auipc");
        return true;
    }

    // ---------------- J-type ----------------
    case 0x6F: { // JAL
        asmOut << "jal " << reg(f.rd) << ", " << f.immJ();
        cout << asmOut.str() << endl;
        checkX0Write(f.rd, "jal");
        return true;
    }

    // ---------------- I-type: JALR ----------------
    case 0x67: {
        if (f.funct3 == 0x0) {
            asmOut << "jalr " << reg(f.rd) << ", " << f.immI() << "(" << reg(f.rs1) << ")";
            cout << asmOut.str() << endl;
            checkX0Write(f.rd, "jalr");
            return true;
        }
        return false;
    }

    // ---------------- B-type: Branches ----------------
    case 0x63: {
        string mnem;
        switch (f.funct3) {
            case 0x0: mnem = "beq";  break;
            case 0x1: mnem = "bne";  break;
            case 0x4: mnem = "blt";  break;
            case 0x5: mnem = "bge";  break;
            case 0x6: mnem = "bltu"; break;
            case 0x7: mnem = "bgeu"; break;
            default: return false;
        }
        asmOut << mnem << " " << reg(f.rs1) << ", " << reg(f.rs2) << ", " << f.immB();
        cout << asmOut.str() << endl;
        return true;
    }

    // ---------------- I-type: Loads ----------------
    case 0x03: {
        string mnem;
        switch (f.funct3) {
            case 0x0: mnem = "lb";  break;
            case 0x1: mnem = "lh";  break;
            case 0x2: mnem = "lw";  break;
            case 0x4: mnem = "lbu"; break;
            case 0x5: mnem = "lhu"; break;
            default: return false;
        }
        asmOut << mnem << " " << reg(f.rd) << ", " << f.immI() << "(" << reg(f.rs1) << ")";
        cout << asmOut.str() << endl;
        checkX0Write(f.rd, mnem);
        return true;
    }

    // ---------------- S-type: Stores ----------------
    case 0x23: {
        string mnem;
        switch (f.funct3) {
            case 0x0: mnem = "sb"; break;
            case 0x1: mnem = "sh"; break;
            case 0x2: mnem = "sw"; break;
            default: return false;
        }
        asmOut << mnem << " " << reg(f.rs2) << ", " << f.immS() << "(" << reg(f.rs1) << ")";
        cout << asmOut.str() << endl;
        return true; // stores never write rd -> no x0 check needed
    }

    // ---------------- I-type: OP-IMM ----------------
    case 0x13: {
        switch (f.funct3) {
            case 0x0: { // ADDI
                asmOut << "addi " << reg(f.rd) << ", " << reg(f.rs1) << ", " << f.immI();
                cout << asmOut.str() << endl;
                checkX0Write(f.rd, "addi");
                return true;
            }
            case 0x2: { // SLTI
                asmOut << "slti " << reg(f.rd) << ", " << reg(f.rs1) << ", " << f.immI();
                cout << asmOut.str() << endl;
                checkX0Write(f.rd, "slti");
                return true;
            }
            case 0x3: { // SLTIU
                asmOut << "sltiu " << reg(f.rd) << ", " << reg(f.rs1) << ", " << f.immI();
                cout << asmOut.str() << endl;
                checkX0Write(f.rd, "sltiu");
                return true;
            }
            case 0x4: { // XORI
                asmOut << "xori " << reg(f.rd) << ", " << reg(f.rs1) << ", " << f.immI();
                cout << asmOut.str() << endl;
                checkX0Write(f.rd, "xori");
                return true;
            }
            case 0x6: { // ORI
                asmOut << "ori " << reg(f.rd) << ", " << reg(f.rs1) << ", " << f.immI();
                cout << asmOut.str() << endl;
                checkX0Write(f.rd, "ori");
                return true;
            }
            case 0x7: { // ANDI
                asmOut << "andi " << reg(f.rd) << ", " << reg(f.rs1) << ", " << f.immI();
                cout << asmOut.str() << endl;
                checkX0Write(f.rd, "andi");
                return true;
            }
            case 0x1: { // SLLI  (funct7 must be 0000000)
                if (f.funct7 == 0x00) {
                    uint32_t shamt = f.rs2; // bits 24:20
                    asmOut << "slli " << reg(f.rd) << ", " << reg(f.rs1) << ", " << shamt;
                    cout << asmOut.str() << endl;
                    checkX0Write(f.rd, "slli");
                    return true;
                }
                return false;
            }
            case 0x5: { // SRLI / SRAI
                uint32_t shamt = f.rs2;
                if (f.funct7 == 0x00) {
                    asmOut << "srli " << reg(f.rd) << ", " << reg(f.rs1) << ", " << shamt;
                    cout << asmOut.str() << endl;
                    checkX0Write(f.rd, "srli");
                    return true;
                } else if (f.funct7 == 0x20) {
                    asmOut << "srai " << reg(f.rd) << ", " << reg(f.rs1) << ", " << shamt;
                    cout << asmOut.str() << endl;
                    checkX0Write(f.rd, "srai");
                    return true;
                }
                return false;
            }
            default: return false;
        }
    }

    // ---------------- R-type: OP ----------------
    case 0x33: {
        string mnem;
        if (f.funct7 == 0x00) {
            switch (f.funct3) {
                case 0x0: mnem = "add";  break;
                case 0x1: mnem = "sll";  break;
                case 0x2: mnem = "slt";  break;
                case 0x3: mnem = "sltu"; break;
                case 0x4: mnem = "xor";  break;
                case 0x5: mnem = "srl";  break;
                case 0x6: mnem = "or";   break;
                case 0x7: mnem = "and";  break;
                default: return false;
            }
        } else if (f.funct7 == 0x20) {
            switch (f.funct3) {
                case 0x0: mnem = "sub"; break;
                case 0x5: mnem = "sra"; break;
                default: return false;
            }
        } else {
            return false;
        }
        asmOut << mnem << " " << reg(f.rd) << ", " << reg(f.rs1) << ", " << reg(f.rs2);
        cout << asmOut.str() << endl;
        checkX0Write(f.rd, mnem);
        return true;
    }

    // ---------------- FENCE ----------------
    case 0x0F: {
        if (f.funct3 == 0x0) {
            cout << "fence" << endl;
            return true;
        }
        return false;
    }

    // ---------------- SYSTEM: ECALL / EBREAK ----------------
    case 0x73: {
        if (f.funct3 == 0x0) {
            int32_t imm = f.immI();
            if (imm == 0) {
                cout << "ecall" << endl;
                return true;
            } else if (imm == 1) {
                cout << "ebreak" << endl;
                return true;
            }
        }
        return false;
    }

    default:
        return false;
    }
}

// ---------------------------------------------------------------------
// Parses an 8-character hex string into a 32-bit word.
// Returns true on success.
// ---------------------------------------------------------------------
static bool parseHexWord(const string& s, uint32_t& out) {
    if (s.size() != 8) return false;
    for (char c : s) {
        if (!isxdigit(static_cast<unsigned char>(c))) return false;
    }
    out = static_cast<uint32_t>(stoul(s, nullptr, 16));
    return true;
}

// ---------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------
int main() {
    string line;

    cout << "RISC-V (RV32I) Instruction Decoder" << endl;
    cout << "Enter an 32-bit RISC-V instruction in hex (e.g., 00510133) or 'quit' to exit." << endl;

    while (true) {
        cout << "\n> ";
        if (!getline(cin, line)) {
            cout << "\nEnd of input. Exiting." << endl;
            break;
        }

        // Trim whitespace
        size_t start = line.find_first_not_of(" \t\r\n");
        size_t end   = line.find_last_not_of(" \t\r\n");
        if (start == string::npos) {
            cout << "No input provided. Please enter a hex instruction or 'quit'." << endl;
            continue;
        }
        string trimmed = line.substr(start, end - start + 1);

        if (trimmed == "quit" || trimmed == "exit") {
            cout << "Exiting." << endl;
            break;
        }

        uint32_t word;
        if (!parseHexWord(trimmed, word)) {
            cout << "Invalid input: \"" << trimmed
                 << "\" is not a valid 8-character hex string. Try again." << endl;
            continue;
        }

        Fields f(word);
        cout << "Instruction 0x" << hex << setw(8) << setfill('0') << word
             << dec << setfill(' ') << " -> ";

        bool ok = decodeInstruction(f);
        if (!ok) {
            printUndecodable(f);
        }
    }

    return 0;
}