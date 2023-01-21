//
//  repl.cc
//  Katara
//
//  Created by Arne Philipeit on 10/2/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "repl.h"

#include "src/common/logging/logging.h"

namespace cmd {

using ::common::logging::fail;

const REPL::Config REPL::kDefaultConfig = Config{.prompt = "> ", .exit_command = "exit"};

REPL::REPL(std::function<void(std::string)> command_executor, Context* ctx, Config cfg)
    : command_executor_(command_executor), ctx_(ctx), cfg_(cfg) {
  if (tcgetattr(0, &old_settings_) < 0) perror("tcsetattr()");
  old_settings_.c_lflag &= ~ICANON;
  old_settings_.c_lflag &= ~ECHO;
  old_settings_.c_cc[VMIN] = 1;
  old_settings_.c_cc[VTIME] = 0;
  if (tcsetattr(0, TCSANOW, &old_settings_) < 0) perror("tcsetattr ICANON");
}

REPL::~REPL() {
  old_settings_.c_lflag |= ICANON;
  old_settings_.c_lflag |= ECHO;
  if (tcsetattr(0, TCSADRAIN, &old_settings_) < 0) perror("tcsetattr ~ICANON");
}

char REPL::ReadChar() { return ctx_->stdin()->get(); }

void REPL::RingBell() {
  *ctx_->stdout() << char(7);
  ctx_->stdout()->flush();
}

void REPL::ClearCurrentCommand() {
  *ctx_->stdout() << "\r" << cfg_.prompt;
  for (std::size_t i = 0; i < command_.size(); i++) {
    *ctx_->stdout() << " ";
  }
  ctx_->stdout()->flush();
}

void REPL::ReprintCurrentCommand() {
  *ctx_->stdout() << "\r" << cfg_.prompt << command_;
  *ctx_->stdout() << "\r" << cfg_.prompt;
  for (std::size_t i = 0; i < cursor_position_; i++) {
    *ctx_->stdout() << command_.at(i);
  }
  ctx_->stdout()->flush();
}

void REPL::HandleArrowUp() {
  if (buffer_position_ == 0) {
    RingBell();
    return;
  }

  ClearCurrentCommand();

  buffer_position_--;
  command_ = command_buffer_.at(buffer_position_);
  cursor_position_ = command_.length();

  ReprintCurrentCommand();
}

void REPL::HandleArrowDown() {
  if (buffer_position_ == command_buffer_.size()) {
    RingBell();
    return;
  }

  ClearCurrentCommand();

  if (buffer_position_ == command_buffer_.size() - 1) {
    buffer_position_++;
    command_ = "";
    cursor_position_ = 0;
  } else {
    buffer_position_++;
    command_ = command_buffer_.at(buffer_position_);
    cursor_position_ = command_.length();
  }

  ReprintCurrentCommand();
}

void REPL::HandleArrowRight() {
  if (cursor_position_ == command_.length()) {
    RingBell();
    return;
  }

  *ctx_->stdout() << command_.at(cursor_position_++);
  ctx_->stdout()->flush();
}

void REPL::HandleArrowLeft() {
  if (cursor_position_ == 0) {
    RingBell();
    return;
  }

  cursor_position_--;

  ReprintCurrentCommand();
}

void REPL::HandleDeleteBackward() {
  if (cursor_position_ == 0) {
    RingBell();
    return;
  }

  ClearCurrentCommand();

  command_ = command_.substr(0, cursor_position_ - 1) + command_.substr(cursor_position_);
  cursor_position_--;

  ReprintCurrentCommand();
}

void REPL::HandleDeleteForward() {
  if (cursor_position_ == command_.length()) {
    RingBell();
    return;
  }

  ClearCurrentCommand();

  command_ = command_.substr(0, cursor_position_) + command_.substr(cursor_position_ + 1);

  ReprintCurrentCommand();
}

bool REPL::HandleEnter() {
  *ctx_->stdout() << "\n";
  if (command_ == cfg_.exit_command) {
    return true;
  } else {
    command_executor_(command_);
  }
  *ctx_->stdout() << cfg_.prompt;
  ctx_->stdout()->flush();

  if (command_.empty()) {
    buffer_position_ = command_buffer_.size();
    return false;
  }
  if (command_buffer_.empty() || command_ != command_buffer_.back()) {
    command_buffer_.push_back(command_);
  }
  buffer_position_ = command_buffer_.size();
  command_ = "";
  cursor_position_ = 0;
  return false;
}

void REPL::HandleInput(char input) {
  command_ = command_.substr(0, cursor_position_) + input + command_.substr(cursor_position_);
  cursor_position_++;

  ReprintCurrentCommand();
}

void REPL::Run() {
  *ctx_->stdout() << cfg_.prompt;
  while (true) {
    switch (char c = ReadChar()) {
      case 0:
        return;
      case 10:
        if (bool should_terminate = HandleEnter(); should_terminate) {
          return;
        }
        break;
      case 127:
        HandleDeleteBackward();
        break;
      case 27:
        switch (char d = ReadChar()) {
          case 0:
            return;
          case 91:
            break;
          default:
            fail("unexpected character in escape sequence: " + std::to_string(d));
        }
        switch (char e = ReadChar()) {
          case 0:
            return;
          case 51:
            switch (char f = ReadChar()) {
              case 0:
                return;
              case 126:
                break;
              default:
                fail("unexpected character in escape sequence: " + std::to_string(f));
            }
            HandleDeleteForward();
            break;
          case 65:
            HandleArrowUp();
            break;
          case 66:
            HandleArrowDown();
            break;
          case 67:
            HandleArrowRight();
            break;
          case 68:
            HandleArrowLeft();
            break;
          default:
            fail("unexpected character in escape sequence: " + std::to_string(e));
        }
        break;
      default:
        HandleInput(c);
        break;
    }
  }
}

void REPL::InterruptOutput(std::function<void()> interruptor) {
  *ctx_->stdout() << "\r";
  interruptor();
  ReprintCurrentCommand();
}

}  // namespace cmd
