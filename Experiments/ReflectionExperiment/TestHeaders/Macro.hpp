#define Component(Name, b) struct Name {
#define Property(...)
#define EndComponent(...)  }

Component(HelloWorld)
Property(Min = 0, Max = 10) float Height	= 0;
Property(Min = 0, Max = 10) float Width		= 0;
EndComponent();



struct Component {};
struct NoProperties {};

typename < typenane TY, typename EditorProperties = NoProperties>
struct Property
{
	explicit Property(auto ... args) value { std::forward<decltype(args)>(args)... } {}

	TY value;

	operator TY& () { return value; }
};

struct ClampedOptions
{
	float min = 0;
	float max = 0;
};

struct HelloWorld : Component
{
	Property<float, ClampedOptions{ .min = 0, .max = 10 }>	height	= 0;
	Property<float, ClampedOptions{ .min = 0, .max = 10 }>	width	= 0;
	Property<float, ClampedOptions{ .min = 0, .max = 10 }>	width	= 0;
};
