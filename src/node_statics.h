#ifndef SRC_NODE_STATICS_H_
#define SRC_NODE_STATICS_H_

#define NODE_STATICS(modname, isolate) (isolate)->statics_.modname

#define NODE_STATICS_NEW(modname, classname, varname) \
  classname *varname = new classname();               \
  NODE_STATICS(modname, node::Isolate::GetCurrent()) = varname;

#define NODE_STATICS_GET(modname, classname)    \
  static_cast<classname *>(NODE_STATICS(modname, node::Isolate::GetCurrent()))

namespace node {

class ModuleStatics {
public:
  inline ModuleStatics() {}
  inline virtual ~ModuleStatics() {}
};

#define NODE_EXT_STATICS_DECL(x) ModuleStatics *x;
#define NODE_EXT_LIST_START typedef struct _ext_statics {
#define NODE_EXT_LIST_ITEM NODE_EXT_STATICS_DECL
#define NODE_EXT_LIST_END                \
  NODE_EXT_STATICS_DECL(node_io_watcher) \
  NODE_EXT_STATICS_DECL(node_stream_wrap) \
  } ext_statics;

#include "node_extensions.h"
    
#undef NODE_EXT_LIST_START
#undef NODE_EXT_LIST_ITEM
#undef NODE_EXT_LIST_END

};

#endif //SRC_NODE_STATICS_H_
