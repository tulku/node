// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef SRC_FILE_H_
#define SRC_FILE_H_

#include <node.h>
//#include <node_statics.h>
#include <v8.h>

namespace node {

class FileStatics : public ModuleStatics {
public:
  v8::Persistent<v8::String> encoding_symbol;
  v8::Persistent<v8::String> errno_symbol;
  v8::Persistent<v8::String> buf_symbol;
  v8::Persistent<v8::String> oncomplete_sym;
  v8::Persistent<v8::String> syscall_symbol;
  v8::Persistent<v8::String> errpath_symbol;
  v8::Persistent<v8::String> code_symbol;
  v8::Persistent<v8::FunctionTemplate> stats_constructor_template;
  v8::Persistent<v8::String> dev_symbol;
  v8::Persistent<v8::String> ino_symbol;
  v8::Persistent<v8::String> mode_symbol;
  v8::Persistent<v8::String> nlink_symbol;
  v8::Persistent<v8::String> uid_symbol;
  v8::Persistent<v8::String> gid_symbol;
  v8::Persistent<v8::String> rdev_symbol;
  v8::Persistent<v8::String> size_symbol;
  v8::Persistent<v8::String> blksize_symbol;
  v8::Persistent<v8::String> blocks_symbol;
  v8::Persistent<v8::String> atime_symbol;
  v8::Persistent<v8::String> mtime_symbol;
  v8::Persistent<v8::String> ctime_symbol;
#ifdef __POSIX__
  v8::Persistent<v8::FunctionTemplate> stat_watcher_constructor_template;
#endif
};
    
class File {
 public:
  static void Initialize(v8::Handle<v8::Object> target);
};

void InitFs(v8::Handle<v8::Object> target);

}  // namespace node
#endif  // SRC_FILE_H_
