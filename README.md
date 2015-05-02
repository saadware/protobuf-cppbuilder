#STATUS - Project Stopped

I don't own C++Builder/RAD Studio and therefore cannot properly maintain
this. 

If I were to pick this up again, I would start with a clean fork as
protobuf has since [moved to GitHub](https://github.com/google/protobuf).
The project was in svn on Google Code when I originally started this.

-------
An attempt to make the Google's Protocol Buffers (protobuf) build in the
Windows based compiler, C++ Builder by Embarcadero.

STATUS: 

* libprotobuf and libprotobuf-lite should all be working now. 
* protoc.exe has some problems when parsing options in a proto file.
  I've spent a bit of time on this and have not yet been able to
  determine what's going on.  For now, use a pre-built protoc.exe to
  generate the actual C++ classes.
* I've tested my changes on both gcc and MSVC and all unit tests still
  pass.
* Unit tests are building now. Next step is to get them all running. 
* For some reason the TEST(WireFormatTest, ZigZag) is not compiling.
  Still need to sort this out.
* Currently, only C++Builder XE is supported. Looking into options on
  how to support multiple versions of the IDE.
