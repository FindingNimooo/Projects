//Filename: simulator.cpp
//Assignment: Project 3: RISC-V Pipeline Simulator Using C++
//Name: Peter Estacio
//Grouped with Ni�o De Mesa and Osh Ong
//Section: ENGG 123.01 - J1

#include "simulator.h"

#include <fstream>
#include <iomanip>
#include <istream>
#include <ostream>
#include <sstream>
#include <utility>

namespace
{
  constexpr uint64_t MAX_CYCLES = 1000000;

  uint32_t bits(uint32_t word, unsigned high, unsigned low)
  {
    return (word >> low) & ((uint32_t{1} << (high - low + 1)) - 1);
  }

  int64_t signExtend(uint32_t value, unsigned width)
  {
    const int64_t sign = int64_t{1} << (width - 1);
    return (static_cast<int64_t>(value) ^ sign) - sign;
  }

  std::string hexValue(uint64_t value, unsigned width = 0)
  {
    std::ostringstream text;
    text << std::hex << std::setfill('0') << std::setw(width) << value;
    return text.str();
  }

  bool parseUnsigned(const std::string& text, unsigned base, uint64_t& value,
    bool allowPrefix = false)
  {
    std::size_t start = 0;
    if (allowPrefix && base == 16 && text.size() >= 2 &&
        text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
    {
      start = 2;
    }
    if (start == text.size())
    {
      return false;
    }

    value = 0;
    for (std::size_t i = start; i < text.size(); ++i)
    {
      unsigned digit = 16;
      if (text[i] >= '0' && text[i] <= '9')
      {
        digit = static_cast<unsigned>(text[i] - '0');
      }
      else if (text[i] >= 'a' && text[i] <= 'f')
      {
        digit = static_cast<unsigned>(text[i] - 'a') + 10;
      }
      else if (text[i] >= 'A' && text[i] <= 'F')
      {
        digit = static_cast<unsigned>(text[i] - 'A') + 10;
      }
      if (digit >= base || value > (UINT64_MAX - digit) / base)
      {
        return false;
      }
      value = value * base + digit;
    }
    return true;
  }

  bool hasExtraInput(std::istringstream& input)
  {
    std::string extra;
    return static_cast<bool>(input >> extra);
  }

  bool readFilename(std::istringstream& input, std::string& filename)
  {
    input >> std::ws;
    if (input.peek() != '"')
    {
      return static_cast<bool>(input >> filename);
    }

    input.get();
    filename.clear();
    char character;
    while (input.get(character))
    {
      if (character == '"')
      {
        return true;
      }
      if (character == '\\' && !input.get(character))
      {
        return false;
      }
      filename += character;
    }
    return false;
  }

  void showHelp(std::ostream& output)
  {
    output << "Available commands:\n"
      << "  help\n"
      << "  loaddata address filename\n"
      << "  showdata address N\n"
      << "  loadcode address filename\n"
      << "  showcode address N\n"
      << "  exec address\n"
      << "  Rn value              (n = 1..31)\n"
      << "  exit\n"
      << "Addresses are hexadecimal byte addresses; N is decimal.\n"
      << "Register values are unsigned decimal or 0x-prefixed hexadecimal.\n"
      << "Quote filenames containing spaces. Data alignment: 8; code: 4.\n";
  }
}

Simulator::Instruction Simulator::decode(Instruction instruction)
{
  if (!instruction.valid || !instruction.error.empty())
  {
    return instruction;
  }

  const uint32_t word = instruction.word;
  const unsigned opcode = bits(word, 6, 0);
  const unsigned funct3 = bits(word, 14, 12);
  const unsigned funct7 = bits(word, 31, 25);
  instruction.rd = bits(word, 11, 7);
  instruction.rs1 = bits(word, 19, 15);
  instruction.rs2 = bits(word, 24, 20);

  if (opcode == 0x13 && funct3 == 0)
  {
    instruction.operation = Operation::AddImmediate;
    instruction.immediate = signExtend(bits(word, 31, 20), 12);
    instruction.readsRs1 = true;
    instruction.writesRd = true;
  }
  else if (opcode == 0x33 && funct3 == 0 && (funct7 == 0 || funct7 == 0x20))
  {
    instruction.operation = funct7 == 0 ? Operation::Add : Operation::Subtract;
    instruction.readsRs1 = true;
    instruction.readsRs2 = true;
    instruction.writesRd = true;
  }
  else if (opcode == 0x33 && funct3 == 0 && funct7 == 1)
  {
    instruction.operation = Operation::Multiply;
    instruction.readsRs1 = true;
    instruction.readsRs2 = true;
    instruction.writesRd = true;
  }
  else if (opcode == 0x03 && funct3 == 3)
  {
    instruction.operation = Operation::Load;
    instruction.immediate = signExtend(bits(word, 31, 20), 12);
    instruction.readsRs1 = true;
    instruction.writesRd = true;
  }
  else if (opcode == 0x23 && funct3 == 3)
  {
    instruction.operation = Operation::Store;
    instruction.immediate = signExtend((bits(word, 31, 25) << 5) |
      bits(word, 11, 7), 12);
    instruction.readsRs1 = true;
    instruction.readsRs2 = true;
  }
  else if (opcode == 0x63 && (funct3 == 6 || funct3 == 7))
  {
    instruction.operation = funct3 == 6 ? Operation::BranchLessUnsigned :
      Operation::BranchGreaterEqualUnsigned;
    instruction.immediate = signExtend((bits(word, 31, 31) << 12) |
      (bits(word, 7, 7) << 11) |
      (bits(word, 30, 25) << 5) |
      (bits(word, 11, 8) << 1), 13);
    instruction.readsRs1 = true;
    instruction.readsRs2 = true;
  }
  else if (opcode == 0x6f)
  {
    instruction.operation = Operation::Jump;
    instruction.immediate = signExtend((bits(word, 31, 31) << 20) |
      (bits(word, 19, 12) << 12) |
      (bits(word, 20, 20) << 11) |
      (bits(word, 30, 21) << 1), 21);
    instruction.writesRd = true;
  }
  else if (word == 0x00100073)
  {
    instruction.operation = Operation::Break;
  }
  else
  {
    instruction.error = "Unsupported instruction 0x" + hexValue(word, 8);
  }
  return instruction;
}

uint64_t Simulator::forwardedValue(unsigned source, uint64_t original) const
{
  if (source == 0)
  {
    return 0;
  }

  const Instruction& recent = exmem.instruction;
  if (recent.valid && recent.error.empty() && recent.writesRd &&
      recent.rd == source && recent.operation != Operation::Load)
  {
    return exmem.result;
  }
  const Instruction& earlier = memwb.instruction;
  if (earlier.valid && earlier.error.empty() && earlier.writesRd && earlier.rd == source)
  {
    return memwb.result;
  }
  return original;
}

void Simulator::instructionFetch(IFID& next)
{
  if (fetchStopped)
  {
    return;
  }

  next.instruction.valid = true;
  next.instruction.pc = pc;
  const auto item = instructionMemory.find(pc);
  if (pc % 4 != 0)
  {
    next.instruction.error = "Misaligned instruction address";
    fetchStopped = true;
  }
  else if (item == instructionMemory.end())
  {
    next.instruction.error = "No instruction loaded";
    fetchStopped = true;
  }
  else
  {
    next.instruction.word = item->second;
    pc += 4;
  }
}

void Simulator::instructionDecode(IDEX& next, bool& stalled)
{
  Instruction instruction = decode(ifid.instruction);
  if (!instruction.valid)
  {
    return;
  }

  const Instruction& producer = idex.instruction;
  if (instruction.error.empty() && producer.valid && producer.error.empty() &&
      producer.operation == Operation::Load && producer.rd != 0 &&
      ((instruction.readsRs1 && instruction.rs1 == producer.rd) ||
      (instruction.readsRs2 && instruction.rs2 == producer.rd)))
  {
    stalled = true;
    return;
  }

  next.instruction = instruction;
  next.operand1 = registers[instruction.rs1];
  next.operand2 = registers[instruction.rs2];
  if (!instruction.error.empty() || instruction.operation == Operation::Break)
  {
    fetchStopped = true;
  }
}

void Simulator::execute(EXMEM& next, bool& redirected, uint64_t& target) const
{
  next.instruction = idex.instruction;
  Instruction& instruction = next.instruction;
  if (!instruction.valid || !instruction.error.empty())
  {
    return;
  }

  const uint64_t operand1 = instruction.readsRs1 ?
    forwardedValue(instruction.rs1, idex.operand1) : 0;
  const uint64_t operand2 = instruction.readsRs2 ?
    forwardedValue(instruction.rs2, idex.operand2) : 0;
  const uint64_t immediate = static_cast<uint64_t>(instruction.immediate);
  bool taken = false;

  switch (instruction.operation)
  {
    case Operation::AddImmediate:
    case Operation::Load:
    case Operation::Store:
    {
      next.result = operand1 + immediate;
      next.storeValue = operand2;
      break;
    }
    case Operation::Add:
    {
      next.result = operand1 + operand2;
      break;
    }
    case Operation::Subtract:
    {
      next.result = operand1 - operand2;
      break;
    }
    case Operation::Multiply:
    {
      next.result = operand1 * operand2;
      break;
    }
    case Operation::BranchLessUnsigned:
    {
      taken = operand1 < operand2;
      break;
    }
    case Operation::BranchGreaterEqualUnsigned:
    {
      taken = operand1 >= operand2;
      break;
    }
    case Operation::Jump:
    {
      next.result = instruction.pc + 4;
      taken = true;
      break;
    }
    case Operation::Break:
    case Operation::Invalid:
    {
      break;
    }
  }

  if (taken)
  {
    target = instruction.pc + immediate;
    if (target % 4 != 0)
    {
      instruction.error = "Misaligned control-flow target 0x" + hexValue(target);
    }
    else
    {
      redirected = true;
    }
  }
}

void Simulator::memoryAccess(MEMWB& next)
{
  next.instruction = exmem.instruction;
  next.result = exmem.result;
  Instruction& instruction = next.instruction;
  if (!instruction.valid || !instruction.error.empty())
  {
    return;
  }
  if (instruction.operation != Operation::Load && instruction.operation != Operation::Store)
  {
    return;
  }

  const uint64_t address = exmem.result;
  const auto item = dataMemory.find(address);
  if (address % 8 != 0)
  {
    instruction.error = "Misaligned data address 0x" + hexValue(address);
  }
  else if (item == dataMemory.end())
  {
    instruction.error = "Uninitialized data address 0x" + hexValue(address);
  }
  else if (instruction.operation == Operation::Load)
  {
    next.result = item->second;
  }
  else
  {
    item->second = exmem.storeValue;
  }
}

bool Simulator::writeBack(uint64_t& retired, std::string& error)
{
  const Instruction& instruction = memwb.instruction;
  if (!instruction.valid)
  {
    return false;
  }
  if (!instruction.error.empty())
  {
    error = instruction.error + " at instruction address 0x" + hexValue(instruction.pc) + ".";
    return true;
  }

  ++retired;
  if (instruction.writesRd && instruction.rd != 0)
  {
    registers[instruction.rd] = memwb.result;
  }
  registers[0] = 0;
  return instruction.operation == Operation::Break;
}

void Simulator::executeCode(uint64_t address, std::ostream& output)
{
  pc = address;
  fetchStopped = false;
  ifid = {};
  idex = {};
  exmem = {};
  memwb = {};
  uint64_t retired = 0;

  for (uint64_t cycle = 1; cycle <= MAX_CYCLES; ++cycle)
  {
    IFID nextIFID;
    IDEX nextIDEX;
    EXMEM nextEXMEM;
    MEMWB nextMEMWB;
    std::string error;
    
    if (writeBack(retired, error))
    {
      if (error.empty())
      {
        output << "Execution completed: ebreak. ";
      }
      else
      {
        output << "Execution stopped: " << error << " ";
      }
      output << cycle << " cycle(s), " << retired << " instruction(s) completed.\n";
      return;
    }

    memoryAccess(nextMEMWB);
    const Instruction& memoryInstruction = nextMEMWB.instruction;
    const bool stopping = memoryInstruction.valid &&
      (!memoryInstruction.error.empty() || memoryInstruction.operation == Operation::Break);

    if (stopping)
    {
      fetchStopped = true;
    }
    else
    {
      bool redirected = false;
      uint64_t target = 0;
      execute(nextEXMEM, redirected, target);
      if (redirected)
      {
        pc = target;
        fetchStopped = false;
      }
      else
      {
        bool stalled = false;
        instructionDecode(nextIDEX, stalled);
        if (stalled)
        {
          nextIFID = ifid;
        }
        else
        {
          instructionFetch(nextIFID);
        }
      }
    }

    ifid = std::move(nextIFID);
    idex = std::move(nextIDEX);
    exmem = std::move(nextEXMEM);
    memwb = std::move(nextMEMWB);
  }

  output << "Execution stopped: maximum cycle count (" << MAX_CYCLES
    << ") reached; unfinished instructions discarded.\n";
  ifid = {};
  idex = {};
  exmem = {};
  memwb = {};
}

void Simulator::loadMemory(uint64_t address, const std::string& filename,
  bool code, std::ostream& output)
{
  std::ifstream file(filename);
  if (!file)
  {
    output << "Could not open file: " << filename << "\n";
    return;
  }

  const uint64_t stride = code ? 4 : 8;
  const std::size_t width = code ? 8 : 16;
  uint64_t count = 0;
  uint64_t lineNumber = 0;
  bool exhausted = false;
  std::string line;
  while (std::getline(file, line))
  {
    ++lineNumber;
    std::istringstream input(line);
    std::string token;
    uint64_t value = 0;
    if (!(input >> token) || token.size() != width || !parseUnsigned(token, 16, value))
    {
      output << "Loading stopped at line " << lineNumber << ": expected "
        << width << " hexadecimal digits.\n";
      break;
    }
    if (exhausted)
    {
      output << "Loading stopped: address range overflow.\n";
      break;
    }

    if (code)
    {
      instructionMemory[address] = static_cast<uint32_t>(value);
    }
    else
    {
      dataMemory[address] = value;
    }
    ++count;
    exhausted = address > UINT64_MAX - stride;
    if (!exhausted)
    {
      address += stride;
    }
  }
  if (file.bad())
  {
    output << "Loading stopped: file read error.\n";
  }
  output << count << (code ? " instruction(s)" : " data item(s)") << " loaded.\n";
}

void Simulator::showMemory(uint64_t address, uint64_t count,
  bool code, std::ostream& output) const
{
  const uint64_t stride = code ? 4 : 8;
  if (count - 1 > (UINT64_MAX - address) / stride)
  {
    output << "Requested address range overflows.\n";
    return;
  }

  for (uint64_t i = 0; i < count; ++i)
  {
    const uint64_t location = address + i * stride;
    output << hexValue(location) << ": ";
    if (code)
    {
      const auto item = instructionMemory.find(location);
      output << (item == instructionMemory.end() ? "--------" : hexValue(item->second, 8));
    }
    else
    {
      const auto item = dataMemory.find(location);
      output << (item == dataMemory.end() ? "----------------" : hexValue(item->second, 16));
    }
    output << "\n";
  }
}

void Simulator::run(std::istream& input, std::ostream& output)
{
  output << "RISC-V Five-Stage Pipeline Simulator\n"
    << "Enter 'help' to display available commands.\n";
  std::string line;
  while (true)
  {
    output << "\n> " << std::flush;
    if (!std::getline(input, line))
    {
      output << "\nEnd of input. Exiting.\n";
      return;
    }

    std::istringstream arguments(line);
    std::string command;
    if (!(arguments >> command))
    {
      continue;
    }
    if (command == "help" || command == "exit")
    {
      if (hasExtraInput(arguments))
      {
        output << "Usage: " << command << "\n";
      }
      else if (command == "exit")
      {
        output << "Exiting simulator.\n";
        return;
      }
      else
      {
        showHelp(output);
      }
    }
    else if (command.size() > 1 && (command[0] == 'R' || command[0] == 'r'))
    {
      uint64_t number = 0;
      uint64_t value = 0;
      std::string token;
      if (!parseUnsigned(command.substr(1), 10, number) || number < 1 || number > 31)
      {
        output << "Invalid register number; use R1 through R31.\n";
        continue;
      }
      if (!(arguments >> token) || hasExtraInput(arguments))
      {
        output << "Usage: Rn value\n";
        continue;
      }
      const bool hexadecimal = token.size() >= 2 && token[0] == '0' &&
        (token[1] == 'x' || token[1] == 'X');
      if (!parseUnsigned(token, hexadecimal ? 16 : 10, value, hexadecimal))
      {
        output << "Invalid unsigned 64-bit register value.\n";
        continue;
      }
      registers[static_cast<std::size_t>(number)] = value;
      output << "R" << number << " = " << hexValue(value, 16) << "\n";
    }
    else if (command == "loaddata" || command == "loadcode" ||
      command == "showdata" || command == "showcode" || command == "exec")
    {
      const bool loading = command == "loaddata" || command == "loadcode";
      const bool showing = command == "showdata" || command == "showcode";
      const bool code = command == "loadcode" || command == "showcode" || command == "exec";
      std::string addressText;
      std::string parameter;
      bool valid = static_cast<bool>(arguments >> addressText);
      if (loading)
      {
        valid = valid && readFilename(arguments, parameter);
      }
      else if (showing)
      {
        valid = valid && static_cast<bool>(arguments >> parameter);
      }
      if (!valid || hasExtraInput(arguments))
      {
        output << "Usage: " << command << " address"
          << (loading ? " filename" : showing ? " N" : "") << "\n";
        continue;
      }

      uint64_t address = 0;
      const uint64_t alignment = code ? 4 : 8;
      if (!parseUnsigned(addressText, 16, address, true))
      {
        output << "Invalid hexadecimal address.\n";
        continue;
      }
      if (address % alignment != 0)
      {
        output << "Address must be aligned to " << alignment << " bytes.\n";
        continue;
      }
      if (loading)
      {
        loadMemory(address, parameter, code, output);
      }
      else if (showing)
      {
        uint64_t count = 0;
        if (!parseUnsigned(parameter, 10, count) || count == 0)
        {
          output << "N must be a positive decimal integer.\n";
          continue;
        }
        showMemory(address, count, code, output);
      }
      else
      {
        executeCode(address, output);
      }
    }
    else
    {
      output << "Invalid command. Enter 'help' for available commands.\n";
    }
  }
}
