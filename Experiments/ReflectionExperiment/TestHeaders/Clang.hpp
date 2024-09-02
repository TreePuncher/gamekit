#define PROPERTY(AA) __attribute__((annotate("Property(" ##AA ")")))
#define PROPERTYFIELD __attribute__((annotate("Field")))

template<typename TY, int i = 0>
struct ComponentBase {};

template<typename TY, int i = 0>
struct BasicComponent_t {};

struct NoProperties {};

struct HelloWorld
{   //asdf
	PROPERTY("Min: 0.0f; Max: 0.0f") float	width;
	PROPERTY("Max: 0.0f; Max: 0.0f") float	height;
};

using HelloWorldComponent = BasicComponent_t<HelloWorld, 1>;


struct HelloWorld2 : public ComponentBase<HelloWorld2>
{
	PROPERTYFIELD HelloWorld* data;
};

