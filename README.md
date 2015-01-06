JSONCXX
=======

### Introduction

`JSONCXX` is an attempt to create template JSON parser and generator with very simple API via combining advantages of two famous libraries, [RapidJSON](https://github.com/miloyip/rapidjson) and [JSONCPP](https://github.com/open-source-parsers/jsoncpp).

### A note

Current version do not fully support JSON specifications.

### Usage at a glance

The `JSONCXX` should be added to your compile include path.

```cpp
#include "jsoncxx.hpp"

int main()
{
    // The contents of glossary.json file can be found fron http://json.org/example
    // parse a JSON file
    jsoncxx::reader reader;
    jsoncxx::value json;

    if (!reader.parse("glossary.json", json)) {
        std::cerr << "Failed to open or parse glossary.json" << std::endl;
        exit(-1);
    }
    
    jsoncxx::value glossary = json["glossary"];

    // object iterator
    std::cout << "\"glossary\" contains ..." << std::endl;
    std::for_each(glossary.asObject().begin(), glossary.asObject().end(), [&](const std::pair<jsoncxx::value, jsoncxx::value>& elem) {
        std::cout << elem.first << " : " << elem.second << std::endl;
    });

    // object access by name, and string access
    std::cout << "\"title\" is " << glossary["title"].asString() << std::endl;

    // array access 
    jsoncxx::value seealso = glossary["GlossDiv"]["GlossList"]["GlossEntry"]["GlossDef"]["GlossSeeAlso"];
    std::cout << "0th element of \"GlossSeeAlso\" is " << seealso[0] << std::endl;
    
    // array iterator
    std::cout << "\"GlossSeeAlso\" contains ..." << std::endl;
    std::for_each(seealso.asArray().begin(), seealso.asArray().end(), [&](const jsoncxx::value& elem) { std::cout << elem << std::endl; });
    
    // natural or real number type
    glossary["number"]["natural"] = 1234;
    glossary["number"]["real"] = 3.14159265359;

    std::cout << "number types :" << glossary["number"] << std::endl;

    // boolean type
    glossary["boolean"] = true;
    std::cout << "boolean type : " << glossary["boolean"] << std::endl;

    // null type
    glossary["nothing"];
    std::cout << "If nothing specified, then type would be null type and value is " << glossary["nothing"] << std::endl;

    std::ofstream fout("save.json");
    fout << glossary;
    fout.close();

    // implicit casting
    jsoncxx::value::string str = glossary["title"];
    jsoncxx::natural natural = glossary["number"]["natural"];
    jsoncxx::real    real    = glossary["number"]["real"];

    // explicit casting for bool type
    bool boolean = (bool)glossary["boolean"];

    // initializer_list example
    glossary["array"] = { 1, 3.141592, true, "a string"};

    return 0;
}
```

