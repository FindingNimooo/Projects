//Filename: main.cpp
//Assignment: Project 2: Interactive Basic RISC-V Simulator
//Name: Osh Ong
//Grouped with Niño De Mesa and Peter Estacio
//Section: ENGG 123.01 - J1

#include <iostream>
#include <sstream>
#include <string>
#include <cstdint>
#include <cctype>
#include <iomanip>
#include <fstream>
#include <unordered_map>

using namespace std;

uint64_t registers[32] = {0};

unordered_map<uint64_t, uint64_t> dataMemory;
unordered_map<uint64_t, uint32_t> instructionMemory;

static uint32_t bits(uint32_t word, int hi, int lo)
{
    uint32_t mask = (hi - lo + 1 == 32)
        ? 0xFFFFFFFFu
        : ((1u << (hi - lo + 1)) - 1u);

    return (word >> lo) & mask;
}

static int32_t signExtend(uint32_t value, int width)
{
    uint32_t signBit = 1u << (width - 1);

    return static_cast<int32_t>((value ^ signBit) - signBit);
}

struct Fields
{
    uint32_t word;
    uint32_t opcode, rd, funct3, rs1, rs2, funct7;

    explicit Fields(uint32_t w) : word(w)
    {
        opcode = bits(w, 6, 0);
        rd = bits(w, 11, 7);
        funct3 = bits(w, 14, 12);
        rs1 = bits(w, 19, 15);
        rs2 = bits(w, 24, 20);
        funct7 = bits(w, 31, 25);
    }

    int32_t immI() const
    {
        return signExtend(bits(word, 31, 20), 12);
    }

    int32_t immS() const
    {
        uint32_t imm =
            (bits(word, 31, 25) << 5) |
            bits(word, 11, 7);

        return signExtend(imm, 12);
    }

    int32_t immB() const
    {
        uint32_t imm =
            (bits(word, 31, 31) << 12) |
            (bits(word, 7, 7) << 11) |
            (bits(word, 30, 25) << 5) |
            (bits(word, 11, 8) << 1);

        return signExtend(imm, 13);
    }

    int32_t immJ() const
    {
        uint32_t imm =
            (bits(word, 31, 31) << 20) |
            (bits(word, 19, 12) << 12) |
            (bits(word, 20, 20) << 11) |
            (bits(word, 30, 21) << 1);

        return signExtend(imm, 21);
    }
};

static bool isHexString(const string& s, int length)
{
    if ((int)s.size() != length)
        return false;

    for (char c : s)
    {
        if (!isxdigit(static_cast<unsigned char>(c)))
            return false;
    }

    return true;
}

static bool parseHexAddress(const string& s, uint64_t& address)
{
    if (s.empty())
        return false;

    for (char c : s)
    {
        if (!isxdigit(static_cast<unsigned char>(c)))
            return false;
    }

    try
    {
        address = stoull(s, nullptr, 16);
        return true;
    }

    catch (...)
    {
        return false;
    }
}

static bool parseRegisterValue(const string& s, uint64_t& value)
{
    try
    {
        if (s.size() > 2 &&
            s[0] == '0' &&
            (s[1] == 'x' || s[1] == 'X'))
        {
            value = stoull(s.substr(2), nullptr, 16);
            return true;
        }

        for (char c : s)
        {
            if (!isdigit(static_cast<unsigned char>(c)))
                return false;
        }

        value = stoull(s, nullptr, 10);
        return true;
    }

    catch (...)
    {
        return false;
    }
}

static void showHelp()
{
    cout << "\nAvailable commands:" << endl;
    cout << "help" << endl;
    cout << "loaddata address filename" << endl;
    cout << "showdata address N" << endl;
    cout << "loadcode address filename" << endl;
    cout << "showcode address N" << endl;
    cout << "exec address" << endl;
    cout << "Rn value" << endl;
    cout << "exit" << endl;
}

static void loadData(uint64_t address, const string& filename)
{
    ifstream file(filename);

    if (!file)
    {
        cout << "Could not open file: " << filename << endl;
        return;
    }

    string line;
    int count = 0;

    while (getline(file, line))
    {
        stringstream ss(line);
        string valueString;

        if (!(ss >> valueString))
            break;

        if (!isHexString(valueString, 16))
        {
            cout << "Loading stopped at invalid data line." << endl;
            break;
        }

        try
        {
            uint64_t value =
                stoull(valueString, nullptr, 16);

            dataMemory[address] = value;

            address++;
            count++;
        }

        catch (...)
        {
            cout << "Loading stopped because of invalid data." << endl;
            break;
        }
    }

    cout << count
         << " data item(s) loaded."
         << endl;
}

static void showData(uint64_t address, int N)
{
    if (N < 0)
    {
        cout << "Invalid number of data items." << endl;
        return;
    }

    for (int i = 0; i < N; i++)
    {
        uint64_t currentAddress = address + i;

        cout << hex
             << currentAddress
             << ": ";

        auto item = dataMemory.find(currentAddress);

        if (item == dataMemory.end())
        {
            cout << "----------------";
        }

        else
        {
            cout << setw(16)
                 << setfill('0')
                 << item->second;
        }

        cout << dec
             << setfill(' ')
             << endl;
    }
}

static void loadCode(uint64_t address, const string& filename)
{
    ifstream file(filename);

    if (!file)
    {
        cout << "Could not open file: " << filename << endl;
        return;
    }

    string line;
    int count = 0;

    while (getline(file, line))
    {
        stringstream ss(line);
        string valueString;

        if (!(ss >> valueString))
            break;

        if (!isHexString(valueString, 8))
        {
            cout << "Loading stopped at invalid code line." << endl;
            break;
        }

        try
        {
            uint32_t value =
                static_cast<uint32_t>(
                    stoul(valueString, nullptr, 16));

            instructionMemory[address] = value;

            address++;
            count++;
        }

        catch (...)
        {
            cout << "Loading stopped because of invalid code." << endl;
            break;
        }
    }

    cout << count
         << " instruction(s) loaded."
         << endl;
}

static void showCode(uint64_t address, int N)
{
    if (N < 0)
    {
        cout << "Invalid number of instructions." << endl;
        return;
    }

    for (int i = 0; i < N; i++)
    {
        uint64_t currentAddress = address + i;

        cout << hex
             << currentAddress
             << ": ";

        auto item = instructionMemory.find(currentAddress);

        if (item == instructionMemory.end())
        {
            cout << "--------";
        }

        else
        {
            cout << setw(8)
                 << setfill('0')
                 << item->second;
        }

        cout << dec
             << setfill(' ')
             << endl;
    }
}

static bool executeInstruction(uint32_t word, uint64_t& pc)
{
    Fields f(word);

    uint64_t nextPC = pc + 1;

    switch (f.opcode)
    {
        case 0x13:
        {
            if (f.funct3 == 0x0)
            {
                if (f.rd != 0)
                {
                    registers[f.rd] =
                        registers[f.rs1] +
                        static_cast<int64_t>(f.immI());
                }
            }

            else
            {
                cout << "Unsupported I-type instruction at address "
                     << hex << pc << dec << "." << endl;

                return false;
            }

            break;
        }

        case 0x33:
        {
            if (f.funct3 == 0x0 && f.funct7 == 0x00)
            {
                if (f.rd != 0)
                {
                    registers[f.rd] =
                        registers[f.rs1] +
                        registers[f.rs2];
                }
            }

            else if (f.funct3 == 0x0 && f.funct7 == 0x20)
            {
                if (f.rd != 0)
                {
                    registers[f.rd] =
                        registers[f.rs1] -
                        registers[f.rs2];
                }
            }

            else
            {
                cout << "Unsupported R-type instruction at address "
                     << hex << pc << dec << "." << endl;

                return false;
            }

            break;
        }

        case 0x03:
        {
            if (f.funct3 != 0x3)
            {
                cout << "Unsupported load instruction at address "
                     << hex << pc << dec << "." << endl;

                return false;
            }

            uint64_t address =
                registers[f.rs1] +
                static_cast<int64_t>(f.immI());

            auto item = dataMemory.find(address);

            if (item == dataMemory.end())
            {
                cout << "Invalid data memory read at address "
                     << hex << address << dec << "." << endl;

                return false;
            }

            if (f.rd != 0)
            {
                registers[f.rd] = item->second;
            }

            break;
        }

        case 0x23:
        {
            if (f.funct3 != 0x3)
            {
                cout << "Unsupported store instruction at address "
                     << hex << pc << dec << "." << endl;

                return false;
            }

            uint64_t address =
                registers[f.rs1] +
                static_cast<int64_t>(f.immS());

            auto item = dataMemory.find(address);

            if (item == dataMemory.end())
            {
                cout << "Invalid data memory write at address "
                     << hex << address << dec << "." << endl;

                return false;
            }

            dataMemory[address] =
                registers[f.rs2];

            break;
        }

        case 0x63:
        {
            bool branch = false;

            if (f.funct3 == 0x6)
            {
                branch =
                    registers[f.rs1] <
                    registers[f.rs2];
            }

            else if (f.funct3 == 0x7)
            {
                branch =
                    registers[f.rs1] >=
                    registers[f.rs2];
            }

            else
            {
                cout << "Unsupported branch instruction at address "
                     << hex << pc << dec << "." << endl;

                return false;
            }

            if (branch)
            {
                nextPC =
                    pc +
                    (f.immB() / 4);
            }

            break;
        }

        case 0x6F:
        {
            if (f.rd != 0)
            {
                registers[f.rd] = pc + 1;
            }

            nextPC =
                pc +
                (f.immJ() / 4);

            break;
        }

        case 0x73:
        {
            if (f.funct3 == 0x0 &&
                f.immI() == 1)
            {
                cout << "Execution completed." << endl;
                return false;
            }

            cout << "Unsupported system instruction at address "
                 << hex << pc << dec << "." << endl;

            return false;
        }

        default:
        {
            cout << "Unsupported instruction at address "
                 << hex << pc << dec << "." << endl;

            return false;
        }
    }

    registers[0] = 0;

    pc = nextPC;

    return true;
}

static void executeCode(uint64_t startAddress)
{
    uint64_t pc = startAddress;

    int instructionCount = 0;

    const int MAX_INSTRUCTIONS = 100000;

    while (instructionCount < MAX_INSTRUCTIONS)
    {
        auto item = instructionMemory.find(pc);

        if (item == instructionMemory.end())
        {
            cout << "Execution stopped: no instruction at address "
                 << hex << pc << dec << "." << endl;

            return;
        }

        uint32_t word = item->second;

        if (!executeInstruction(word, pc))
            return;

        instructionCount++;
    }

    cout << "Execution stopped: maximum instruction count reached."
         << endl;
}

int main()
{
    string line;

    cout << "Interactive Basic RISC-V Simulator" << endl;
    cout << "Enter 'help' to display available commands." << endl;

    while (true)
    {
        cout << "\n> ";

        if (!getline(cin, line))
        {
            cout << "\nEnd of input. Exiting." << endl;
            break;
        }

        stringstream ss(line);

        string command;

        ss >> command;

        if (command.empty())
            continue;

        if (command == "help")
        {
            showHelp();
        }

        else if (command == "exit")
        {
            cout << "Exiting simulator." << endl;
            break;
        }

        else if (command == "loaddata")
        {
            string addressString;
            string filename;

            if (!(ss >> addressString >> filename))
            {
                cout << "Invalid input." << endl;
                cout << "Usage: loaddata address filename" << endl;
                continue;
            }

            uint64_t address;

            if (!parseHexAddress(addressString, address))
            {
                cout << "Invalid hexadecimal address." << endl;
                continue;
            }

            loadData(address, filename);
        }

        else if (command == "showdata")
        {
            string addressString;
            int N;

            if (!(ss >> addressString >> N))
            {
                cout << "Invalid input." << endl;
                cout << "Usage: showdata address N" << endl;
                continue;
            }

            uint64_t address;

            if (!parseHexAddress(addressString, address))
            {
                cout << "Invalid hexadecimal address." << endl;
                continue;
            }

            showData(address, N);
        }

        else if (command == "loadcode")
        {
            string addressString;
            string filename;

            if (!(ss >> addressString >> filename))
            {
                cout << "Invalid input." << endl;
                cout << "Usage: loadcode address filename" << endl;
                continue;
            }

            uint64_t address;

            if (!parseHexAddress(addressString, address))
            {
                cout << "Invalid hexadecimal address." << endl;
                continue;
            }

            loadCode(address, filename);
        }

        else if (command == "showcode")
        {
            string addressString;
            int N;

            if (!(ss >> addressString >> N))
            {
                cout << "Invalid input." << endl;
                cout << "Usage: showcode address N" << endl;
                continue;
            }

            uint64_t address;

            if (!parseHexAddress(addressString, address))
            {
                cout << "Invalid hexadecimal address." << endl;
                continue;
            }

            showCode(address, N);
        }

        else if (command == "exec")
        {
            string addressString;

            if (!(ss >> addressString))
            {
                cout << "Invalid input." << endl;
                cout << "Usage: exec address" << endl;
                continue;
            }

            uint64_t address;

            if (!parseHexAddress(addressString, address))
            {
                cout << "Invalid hexadecimal address." << endl;
                continue;
            }

            executeCode(address);
        }

        else if ((command[0] == 'R' || command[0] == 'r')
                 && command.size() > 1)
        {
            try
            {
                int registerNumber =
                    stoi(command.substr(1));

                if (registerNumber < 1 ||
                    registerNumber > 31)
                {
                    cout << "Invalid register number." << endl;
                    continue;
                }

                string valueString;

                if (!(ss >> valueString))
                {
                    cout << "Invalid input." << endl;
                    cout << "Usage: Rn value" << endl;
                    continue;
                }

                uint64_t value;

                if (!parseRegisterValue(valueString, value))
                {
                    cout << "Invalid register value." << endl;
                    continue;
                }

                registers[registerNumber] = value;

                cout << "R"
                     << registerNumber
                     << " = "
                     << hex
                     << setw(16)
                     << setfill('0')
                     << registers[registerNumber]
                     << dec
                     << setfill(' ')
                     << endl;
            }

            catch (...)
            {
                cout << "Invalid register command." << endl;
            }
        }

        else
        {
            cout << "Invalid command. Enter 'help' for available commands."
                 << endl;
        }

        registers[0] = 0;
    }

    return 0;
}
