//Filename: simulator.h
//Assignment: Project 3: RISC-V Pipeline Simulator Using C++
//Name: Peter Estacio
//Grouped with Ni�o De Mesa and Osh Ong
//Section: ENGG 123.01 - J1

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <array>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <unordered_map>

class Simulator
{
public:
  void run(std::istream& input, std::ostream& output);

private:
  enum class Operation
  {
    Invalid, AddImmediate, Add, Subtract, Load, Store,
    BranchLessUnsigned, BranchGreaterEqualUnsigned, Jump, Break
  };

  struct Instruction
  {
    bool valid = false;
    uint64_t pc = 0;
    uint32_t word = 0;
    Operation operation = Operation::Invalid;
    unsigned rd = 0;
    unsigned rs1 = 0;
    unsigned rs2 = 0;
    int64_t immediate = 0;
    bool readsRs1 = false;
    bool readsRs2 = false;
    bool writesRd = false;
    std::string error;
  };

  struct IFID
  {
    Instruction instruction;
  };

  struct IDEX
  {
    Instruction instruction;
    uint64_t operand1 = 0;
    uint64_t operand2 = 0;
  };

  struct EXMEM
  {
    Instruction instruction;
    uint64_t result = 0;
    uint64_t storeValue = 0;
  };

  struct MEMWB
  {
    Instruction instruction;
    uint64_t result = 0;
  };

  std::array<uint64_t, 32> registers{};
  std::unordered_map<uint64_t, uint64_t> dataMemory;
  std::unordered_map<uint64_t, uint32_t> instructionMemory;
  uint64_t pc = 0;
  bool fetchStopped = false;
  IFID ifid;
  IDEX idex;
  EXMEM exmem;
  MEMWB memwb;

  static Instruction decode(Instruction instruction);
  uint64_t forwardedValue(unsigned source, uint64_t original) const;
  void instructionFetch(IFID& next);
  void instructionDecode(IDEX& next, bool& stalled);
  void execute(EXMEM& next, bool& redirected, uint64_t& target) const;
  void memoryAccess(MEMWB& next);
  bool writeBack(uint64_t& retired, std::string& error);
  void executeCode(uint64_t address, std::ostream& output);
  void loadMemory(uint64_t address, const std::string& filename,
                  bool code, std::ostream& output);
  void showMemory(uint64_t address, uint64_t count,
                  bool code, std::ostream& output) const;
};

#endif
