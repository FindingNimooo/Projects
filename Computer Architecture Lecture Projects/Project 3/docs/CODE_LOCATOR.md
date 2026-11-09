# Project 3 Code Locator

Line numbers are inclusive and refer to the finalized source files in the
Project 3 directory. Each stage is a separate member function of `Simulator`.

## Processes Simulating the RISC-V Pipeline Stages

| Pipeline stage | Filename | Line numbers | Function |
|---|---|---|---|
| IF: Instruction fetch | `simulator.cpp` | 191–217 | `Simulator::instructionFetch` |
| ID: Instruction decode and register file read | `simulator.cpp` | 219–244 | `Simulator::instructionDecode` |
| EX: Execution or address calculation | `simulator.cpp` | 246–317 | `Simulator::execute` |
| MEM: Data memory access | `simulator.cpp` | 319–352 | `Simulator::memoryAccess` |
| WB: Write back | `simulator.cpp` | 354–374 | `Simulator::writeBack` |

## Supporting Code

| Responsibility | Filename | Line numbers | Function |
|---|---|---|---|
| Decode instruction fields, immediates, and control signals for ID | `simulator.cpp` | 94–167 | `Simulator::decode` |
| Select forwarded operands for EX | `simulator.cpp` | 169–189 | `Simulator::forwardedValue` |
| Advance the clock, commit next pipeline state, stall, flush, and stop | `simulator.cpp` | 376–459 | `Simulator::executeCode` |
| Load instruction/data input files | `simulator.cpp` | 461–515 | `Simulator::loadMemory` |
| Display instruction/data memory | `simulator.cpp` | 517–543 | `Simulator::showMemory` |
| Process interactive commands and register setup | `simulator.cpp` | 545–666 | `Simulator::run` |

The stage functions operate on the current pipeline registers and populate
the next registers. `executeCode` coordinates them within one simulated cycle;
it does not execute an entire instruction directly. The `decode` helper is part
of ID, and `forwardedValue` is part of EX. Pipeline register types, the register
file, and separate memory maps are declared in `simulator.h`.

Update this table if source edits change line numbers before submission.
