//
//  repl.h
//  Katara
//
//  Created by Arne Philipeit on 10/2/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef cmd_repl_h
#define cmd_repl_h

#include <termios.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "src/cmd/context.h"

namespace cmd {

class REPL {
 public:
  struct Config {
    std::string prompt;
    std::string exit_command;
  };
  static const Config kDefaultConfig;

  REPL(std::function<void(std::string)> command_executor, Context* ctx, Config cfg);
  ~REPL();

  REPL(REPL&&) = delete;
  REPL(const REPL&) = delete;

  REPL& operator=(const REPL&) = delete;
  REPL& operator=(REPL&&) = delete;

  void Run();
  void InterruptOutput(std::function<void()> interruptor);

 private:
  char ReadChar();

  void RingBell();
  void ClearCurrentCommand();
  void ReprintCurrentCommand();

  void HandleArrowUp();
  void HandleArrowDown();
  void HandleArrowRight();
  void HandleArrowLeft();
  void HandleDeleteBackward();
  void HandleDeleteForward();
  bool HandleEnter();
  void HandleInput(char input);

  std::function<void(std::string)> command_executor_;

  std::vector<std::string> command_buffer_;
  std::size_t buffer_position_ = 0;

  std::string command_;
  std::size_t cursor_position_ = 0;

  Context* ctx_;
  Config cfg_;

  struct termios old_settings_;
};

}  // namespace cmd

#endif /* cmd_repl_h */
