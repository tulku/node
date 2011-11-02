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

#include <node_http_parser.h>

#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include <http_parser.h>

#include <string.h>  /* strdup() */
#if !defined(_MSC_VER)
#include <strings.h>  /* strcasecmp() */
#else
#define strcasecmp _stricmp
#endif
#include <stdlib.h>  /* free() */

// This is a binding to http_parser (http://github.com/ry/http-parser)
// The goal is to decouple sockets from parsing for more javascript-level
// agility. A Buffer is read from a socket and passed to parser.execute().
// The parser then issues callbacks with slices of the data
//     parser.onMessageBegin
//     parser.onPath
//     parser.onBody
//     ...
// No copying is performed when slicing the buffer, only small reference
// allocations.


namespace node {

using namespace v8;
    
class HttpStatics : public ModuleStatics {
public:
    Persistent<String> on_headers_sym;
    Persistent<String> on_headers_complete_sym;
    Persistent<String> on_body_sym;
    Persistent<String> on_message_complete_sym;
    Persistent<String> delete_sym;
    Persistent<String> get_sym;
    Persistent<String> head_sym;
    Persistent<String> post_sym;
    Persistent<String> put_sym;
    Persistent<String> connect_sym;
    Persistent<String> options_sym;
    Persistent<String> trace_sym;
    Persistent<String> patch_sym;
    Persistent<String> copy_sym;
    Persistent<String> lock_sym;
    Persistent<String> mkcol_sym;
    Persistent<String> move_sym;
    Persistent<String> propfind_sym;
    Persistent<String> proppatch_sym;
    Persistent<String> unlock_sym;
    Persistent<String> report_sym;
    Persistent<String> mkactivity_sym;
    Persistent<String> checkout_sym;
    Persistent<String> merge_sym;
    Persistent<String> msearch_sym;
    Persistent<String> notify_sym;
    Persistent<String> subscribe_sym;
    Persistent<String> unsubscribe_sym;
    Persistent<String> unknown_method_sym;
    Persistent<String> method_sym;
    Persistent<String> status_code_sym;
    Persistent<String> http_version_sym;
    Persistent<String> version_major_sym;
    Persistent<String> version_minor_sym;
    Persistent<String> should_keep_alive_sym;
    Persistent<String> upgrade_sym;
    Persistent<String> headers_sym;
    Persistent<String> url_sym;
    struct http_parser_settings settings;
    // This is a hack to get the current_buffer to the callbacks with the least
    // amount of overhead. Nothing else will run while http_parser_execute()
    // runs, therefore this pointer can be set and used for the execution.
    Local<Value>* current_buffer;
    char* current_buffer_data;
    size_t current_buffer_len;
    HttpStatics() {
      current_buffer = 0;
      current_buffer_data = 0;
    }
};



#if defined(__GNUC__)
#define always_inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define always_inline __forceinline
#else
#define always_inline
#endif


#define HTTP_CB(name)                                               \
	  static int name(http_parser* p_) {                              \
	    Parser* self = container_of(p_, Parser, parser_);             \
	    return self->name##_();                                       \
	  }                                                               \
	  int always_inline name##_()


#define HTTP_DATA_CB(name)                                          \
  static int name(http_parser* p_, const char* at, size_t length) { \
    Parser* self = container_of(p_, Parser, parser_);               \
    return self->name##_(at, length);                               \
  }                                                                 \
  int always_inline name##_(const char* at, size_t length)


static inline Persistent<String>
method_to_str(unsigned short m) {
  HttpStatics *statics = NODE_STATICS_GET(node_http_parser, HttpStatics);
  switch (m) {
    case HTTP_DELETE:     return statics->delete_sym;
    case HTTP_GET:        return statics->get_sym;
    case HTTP_HEAD:       return statics->head_sym;
    case HTTP_POST:       return statics->post_sym;
    case HTTP_PUT:        return statics->put_sym;
    case HTTP_CONNECT:    return statics->connect_sym;
    case HTTP_OPTIONS:    return statics->options_sym;
    case HTTP_TRACE:      return statics->trace_sym;
    case HTTP_PATCH:      return statics->patch_sym;
    case HTTP_COPY:       return statics->copy_sym;
    case HTTP_LOCK:       return statics->lock_sym;
    case HTTP_MKCOL:      return statics->mkcol_sym;
    case HTTP_MOVE:       return statics->move_sym;
    case HTTP_PROPFIND:   return statics->propfind_sym;
    case HTTP_PROPPATCH:  return statics->proppatch_sym;
    case HTTP_UNLOCK:     return statics->unlock_sym;
    case HTTP_REPORT:     return statics->report_sym;
    case HTTP_MKACTIVITY: return statics->mkactivity_sym;
    case HTTP_CHECKOUT:   return statics->checkout_sym;
    case HTTP_MERGE:      return statics->merge_sym;
    case HTTP_MSEARCH:    return statics->msearch_sym;
    case HTTP_NOTIFY:     return statics->notify_sym;
    case HTTP_SUBSCRIBE:  return statics->subscribe_sym;
    case HTTP_UNSUBSCRIBE:return statics->unsubscribe_sym;
    default:              return statics->unknown_method_sym;
  }
}


// helper class for the Parser
struct StringPtr {
  StringPtr() {
    on_heap_ = false;
    Reset();
  }


  ~StringPtr() {
    Reset();
  }


  void Reset() {
    if (on_heap_) {
      delete[] str_;
      on_heap_ = false;
    }

    str_ = NULL;
    size_ = 0;
  }


  void Update(const char* str, size_t size) {
    if (str_ == NULL)
      str_ = str;
    else if (on_heap_ || str_ + size != str) {
      // Non-consecutive input, make a copy on the heap.
      // TODO Use slab allocation, O(n) allocs is bad.
      char* s = new char[size_ + size];
      memcpy(s, str_, size_);
      memcpy(s + size_, str, size);

      if (on_heap_)
        delete[] str_;
      else
        on_heap_ = true;

      str_ = s;
    }
    size_ += size;
  }


  Handle<String> ToString() const {
    if (str_)
      return String::New(str_, size_);
    else
      return String::Empty();
  }


  const char* str_;
  bool on_heap_;
  size_t size_;
};


class Parser : public ObjectWrap {
public:
  Parser(enum http_parser_type type) : ObjectWrap() {
    Init(type);
  }


  ~Parser() {
  }


  HTTP_CB(on_message_begin) {
    num_fields_ = num_values_ = -1;
    url_.Reset();
    return 0;
  }


  HTTP_DATA_CB(on_url) {
    url_.Update(at, length);
    return 0;
  }


  HTTP_DATA_CB(on_header_field) {
    if (num_fields_ == num_values_) {
      // start of new field name
      if (++num_fields_ == ARRAY_SIZE(fields_)) {
        Flush();
        num_fields_ = 0;
        num_values_ = -1;
      }
      fields_[num_fields_].Reset();
    }

    assert(num_fields_ < (int)ARRAY_SIZE(fields_));
    assert(num_fields_ == num_values_ + 1);

    fields_[num_fields_].Update(at, length);

    return 0;
  }


  HTTP_DATA_CB(on_header_value) {
    if (num_values_ != num_fields_) {
      // start of new header value
      values_[++num_values_].Reset();
    }

    assert(num_values_ < (int)ARRAY_SIZE(values_));
    assert(num_values_ == num_fields_);

    values_[num_values_].Update(at, length);

    return 0;
  }


  HTTP_CB(on_headers_complete) {
    HttpStatics *statics = NODE_STATICS_GET(node_http_parser, HttpStatics);
    Local<Value> cb = handle_->Get(statics->on_headers_complete_sym);

    if (!cb->IsFunction())
      return 0;

    Local<Object> message_info = Object::New();

    if (have_flushed_) {
      // Slow case, flush remaining headers.
      Flush();
    }
    else {
      // Fast case, pass headers and URL to JS land.
      message_info->Set(statics->headers_sym, CreateHeaders());
      if (parser_.type == HTTP_REQUEST)
        message_info->Set(statics->url_sym, url_.ToString());
    }
    num_fields_ = num_values_ = -1;

    // METHOD
    if (parser_.type == HTTP_REQUEST) {
      message_info->Set(statics->method_sym, method_to_str(parser_.method));
    }

    // STATUS
    if (parser_.type == HTTP_RESPONSE) {
      message_info->Set(statics->status_code_sym, Integer::New(parser_.status_code));
    }

    // VERSION
    message_info->Set(statics->version_major_sym, Integer::New(parser_.http_major));
    message_info->Set(statics->version_minor_sym, Integer::New(parser_.http_minor));

    message_info->Set(statics->should_keep_alive_sym,
        http_should_keep_alive(&parser_) ? True() : False());

    message_info->Set(statics->upgrade_sym, parser_.upgrade ? True() : False());

    Local<Value> argv[1] = { message_info };

    Local<Value> head_response =
        Local<Function>::Cast(cb)->Call(handle_, 1, argv);

    if (head_response.IsEmpty()) {
      got_exception_ = true;
      return -1;
    }

    return head_response->IsTrue() ? 1 : 0;
  }


  HTTP_DATA_CB(on_body) {
    HandleScope scope;
    HttpStatics *statics = NODE_STATICS_GET(node_http_parser, HttpStatics);

    Local<Value> cb = handle_->Get(statics->on_body_sym);
    if (!cb->IsFunction())
      return 0;

    Handle<Value> argv[3] = {
      *statics->current_buffer,
      Integer::New(at - statics->current_buffer_data),
      Integer::New(length)
    };

    Local<Value> r = Local<Function>::Cast(cb)->Call(handle_, 3, argv);

    if (r.IsEmpty()) {
      got_exception_ = true;
      return -1;
    }

    return 0;
  }


  HTTP_CB(on_message_complete) {
    HandleScope scope;
    HttpStatics *statics = NODE_STATICS_GET(node_http_parser, HttpStatics);

    if (num_fields_ != -1)
      Flush(); // Flush trailing HTTP headers.

    Local<Value> cb = handle_->Get(statics->on_message_complete_sym);

    if (!cb->IsFunction())
      return 0;

    Local<Value> r = Local<Function>::Cast(cb)->Call(handle_, 0, NULL);

    if (r.IsEmpty()) {
      got_exception_ = true;
      return -1;
    }

    return 0;
  }


  static Handle<Value> New(const Arguments& args) {
    HandleScope scope;

    http_parser_type type =
        static_cast<http_parser_type>(args[0]->Int32Value());

    if (type != HTTP_REQUEST && type != HTTP_RESPONSE) {
      return ThrowException(Exception::Error(String::New(
          "Argument must be HTTPParser.REQUEST or HTTPParser.RESPONSE")));
    }

    Parser* parser = new Parser(type);
    parser->Wrap(args.This());

    return args.This();
  }


  // var bytesParsed = parser->execute(buffer, off, len);
  static Handle<Value> Execute(const Arguments& args) {
    HandleScope scope;
    HttpStatics *statics = NODE_STATICS_GET(node_http_parser, HttpStatics);

    Parser* parser = ObjectWrap::Unwrap<Parser>(args.This());

    assert(!statics->current_buffer);
    assert(!statics->current_buffer_data);

    if (statics->current_buffer) {
      return ThrowException(Exception::TypeError(
            String::New("Already parsing a buffer")));
    }

    Local<Value> buffer_v = args[0];

    if (!Buffer::HasInstance(buffer_v)) {
      return ThrowException(Exception::TypeError(
            String::New("Argument should be a buffer")));
    }

    Local<Object> buffer_obj = buffer_v->ToObject();
    char *buffer_data = Buffer::Data(buffer_obj);
    size_t buffer_len = Buffer::Length(buffer_obj);

    size_t off = args[1]->Int32Value();
    if (off >= buffer_len) {
      return ThrowException(Exception::Error(
            String::New("Offset is out of bounds")));
    }

    size_t len = args[2]->Int32Value();
    if (off+len > buffer_len) {
      return ThrowException(Exception::Error(
            String::New("Length is extends beyond buffer")));
    }

    // Assign 'buffer_' while we parse. The callbacks will access that varible.
    statics->current_buffer = &buffer_v;
    statics->current_buffer_data = buffer_data;
    statics->current_buffer_len = buffer_len;
    parser->got_exception_ = false;

    size_t nparsed =
      http_parser_execute(&parser->parser_, &statics->settings, buffer_data + off, len);

    // Unassign the 'buffer_' variable
    assert(statics->current_buffer);
    statics->current_buffer = NULL;
    statics->current_buffer_data = NULL;

    // If there was an exception in one of the callbacks
    if (parser->got_exception_) return Local<Value>();

    Local<Integer> nparsed_obj = Integer::New(nparsed);
    // If there was a parse error in one of the callbacks
    // TODO What if there is an error on EOF?
    if (!parser->parser_.upgrade && nparsed != len) {
      Local<Value> e = Exception::Error(String::NewSymbol("Parse Error"));
      Local<Object> obj = e->ToObject();
      obj->Set(String::NewSymbol("bytesParsed"), nparsed_obj);
      return scope.Close(e);
    } else {
      return scope.Close(nparsed_obj);
    }
  }


  static Handle<Value> Finish(const Arguments& args) {
    HandleScope scope;
    HttpStatics *statics = NODE_STATICS_GET(node_http_parser, HttpStatics);

    Parser* parser = ObjectWrap::Unwrap<Parser>(args.This());

    assert(!statics->current_buffer);
    parser->got_exception_ = false;

    int rv = http_parser_execute(&(parser->parser_), &statics->settings, NULL, 0);

    if (parser->got_exception_) return Local<Value>();

    if (rv != 0) {
      Local<Value> e = Exception::Error(String::NewSymbol("Parse Error"));
      Local<Object> obj = e->ToObject();
      obj->Set(String::NewSymbol("bytesParsed"), Integer::New(0));
      return scope.Close(e);
    }

    return Undefined();
  }


  static Handle<Value> Reinitialize(const Arguments& args) {
    HandleScope scope;

    http_parser_type type =
        static_cast<http_parser_type>(args[0]->Int32Value());

    if (type != HTTP_REQUEST && type != HTTP_RESPONSE) {
      return ThrowException(Exception::Error(String::New(
          "Argument must be HTTPParser.REQUEST or HTTPParser.RESPONSE")));
    }

    Parser* parser = ObjectWrap::Unwrap<Parser>(args.This());
    parser->Init(type);

    return Undefined();
  }


private:

  Local<Array> CreateHeaders() {
    // num_values_ is either -1 or the entry # of the last header
    // so num_values_ == 0 means there's a single header
    Local<Array> headers = Array::New(2 * (num_values_ + 1));

    for (int i = 0; i < num_values_ + 1; ++i) {
      headers->Set(2 * i, fields_[i].ToString());
      headers->Set(2 * i + 1, values_[i].ToString());
    }

    return headers;
  }


  // spill headers and request path to JS land
  void Flush() {
    HandleScope scope;
    HttpStatics *statics = NODE_STATICS_GET(node_http_parser, HttpStatics);

    Local<Value> cb = handle_->Get(statics->on_headers_sym);

    if (!cb->IsFunction())
      return;

    Handle<Value> argv[2] = {
      CreateHeaders(),
      url_.ToString()
    };

    Local<Value> r = Local<Function>::Cast(cb)->Call(handle_, 2, argv);

    if (r.IsEmpty())
      got_exception_ = true;

    url_.Reset();
    have_flushed_ = true;
  }


  void Init(enum http_parser_type type) {
    http_parser_init(&parser_, type);
    url_.Reset();
    num_fields_ = -1;
    num_values_ = -1;
    have_flushed_ = false;
    got_exception_ = false;
  }


  http_parser parser_;
  StringPtr fields_[32];  // header fields
  StringPtr values_[32];  // header values
  StringPtr url_;
  int num_fields_;
  int num_values_;
  bool have_flushed_;
  bool got_exception_;
};


void InitHttpParser(Handle<Object> target) {
  HandleScope scope;
  NODE_STATICS_NEW(node_http_parser, HttpStatics, statics);

  Local<FunctionTemplate> t = FunctionTemplate::New(Parser::New);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(String::NewSymbol("HTTPParser"));

  PropertyAttribute attrib = (PropertyAttribute) (ReadOnly | DontDelete);
  t->Set(String::NewSymbol("REQUEST"), Integer::New(HTTP_REQUEST), attrib);
  t->Set(String::NewSymbol("RESPONSE"), Integer::New(HTTP_RESPONSE), attrib);

  NODE_SET_PROTOTYPE_METHOD(t, "execute", Parser::Execute);
  NODE_SET_PROTOTYPE_METHOD(t, "finish", Parser::Finish);
  NODE_SET_PROTOTYPE_METHOD(t, "reinitialize", Parser::Reinitialize);

  target->Set(String::NewSymbol("HTTPParser"), t->GetFunction());

  statics->on_headers_sym          = NODE_PSYMBOL("onHeaders");
  statics->on_headers_complete_sym = NODE_PSYMBOL("onHeadersComplete");
  statics->on_body_sym             = NODE_PSYMBOL("onBody");
  statics->on_message_complete_sym = NODE_PSYMBOL("onMessageComplete");

  statics->delete_sym = NODE_PSYMBOL("DELETE");
  statics->get_sym = NODE_PSYMBOL("GET");
  statics->head_sym = NODE_PSYMBOL("HEAD");
  statics->post_sym = NODE_PSYMBOL("POST");
  statics->put_sym = NODE_PSYMBOL("PUT");
  statics->connect_sym = NODE_PSYMBOL("CONNECT");
  statics->options_sym = NODE_PSYMBOL("OPTIONS");
  statics->trace_sym = NODE_PSYMBOL("TRACE");
  statics->patch_sym = NODE_PSYMBOL("PATCH");
  statics->copy_sym = NODE_PSYMBOL("COPY");
  statics->lock_sym = NODE_PSYMBOL("LOCK");
  statics->mkcol_sym = NODE_PSYMBOL("MKCOL");
  statics->move_sym = NODE_PSYMBOL("MOVE");
  statics->propfind_sym = NODE_PSYMBOL("PROPFIND");
  statics->proppatch_sym = NODE_PSYMBOL("PROPPATCH");
  statics->unlock_sym = NODE_PSYMBOL("UNLOCK");
  statics->report_sym = NODE_PSYMBOL("REPORT");
  statics->mkactivity_sym = NODE_PSYMBOL("MKACTIVITY");
  statics->checkout_sym = NODE_PSYMBOL("CHECKOUT");
  statics->merge_sym = NODE_PSYMBOL("MERGE");
  statics->msearch_sym = NODE_PSYMBOL("M-SEARCH");
  statics->notify_sym = NODE_PSYMBOL("NOTIFY");
  statics->subscribe_sym = NODE_PSYMBOL("SUBSCRIBE");
  statics->unsubscribe_sym = NODE_PSYMBOL("UNSUBSCRIBE");;
  statics->unknown_method_sym = NODE_PSYMBOL("UNKNOWN_METHOD");

  statics->method_sym = NODE_PSYMBOL("method");
  statics->status_code_sym = NODE_PSYMBOL("statusCode");
  statics->http_version_sym = NODE_PSYMBOL("httpVersion");
  statics->version_major_sym = NODE_PSYMBOL("versionMajor");
  statics->version_minor_sym = NODE_PSYMBOL("versionMinor");
  statics->should_keep_alive_sym = NODE_PSYMBOL("shouldKeepAlive");
  statics->upgrade_sym = NODE_PSYMBOL("upgrade");
  statics->headers_sym = NODE_PSYMBOL("headers");
  statics->url_sym = NODE_PSYMBOL("url");

  statics->settings.on_message_begin    = Parser::on_message_begin;
  statics->settings.on_url              = Parser::on_url;
  statics->settings.on_header_field     = Parser::on_header_field;
  statics->settings.on_header_value     = Parser::on_header_value;
  statics->settings.on_headers_complete = Parser::on_headers_complete;
  statics->settings.on_body             = Parser::on_body;
  statics->settings.on_message_complete = Parser::on_message_complete;
}

}  // namespace node

NODE_MODULE(node_http_parser, node::InitHttpParser);
