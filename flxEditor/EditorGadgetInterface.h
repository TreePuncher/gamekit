#pragma once
#include <string>

class iEditorGadget
{
public:
    virtual std::string GadgetID() { return "!!!!";  }
    virtual void        Execute() {}
};
