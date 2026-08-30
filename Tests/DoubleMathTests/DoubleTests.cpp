#include <MathUtilities.hpp>


using namespace FlexKit;

int Float4x42Double4x4Constructions()
{
    float4x4 A{
        0, 1, 2, 3,
        4, 5, 6, 7,
        8, 9, 10, 11,
        12, 13, 14, 15
    };
    
    double4x4 B{A};

    double4x4 C{
        0, 1, 2, 3,
        4, 5, 6, 7, 
        8, 1, 2, 3, 
        4, 5, 6, 7
    };

    float4x4 d{ A };
    
    return 0;
}

int Double4x4Mul()
{
    double4x4 A{
        1, 0, 0, 0, 
        0, 2, 0, 0, 
        0, 0, 3, 0, 
        0, 0, 0, 4
    };

    double4x4 res = A * A;

    return 0;
}


int DoubleFloat4x4Mul()
{
    double4x4 A{
        1, 0, 0, 0,
        0, 2, 0, 0,
        0, 0, 3, 0,
        0, 0, 0, 4
    };

    float4x4 B{
        1, 0, 0, 0, 
        0, 2, 0, 0, 
        0, 0, 3, 0,
        0, 0, 0, 4
    };

    double4x4 res = A * B;

    return 0;
}


int CrossPrecisionDotProducts()
{
    float3 xyz{ 1, 2, 3 };
    double3 abc{ 1, 4, 8 };


    double u = abc.dot(xyz);
    float t = xyz.dot(abc);

    return 0;
}

int CrossPrecisionMatrixMul()
{
    double4x4 A{
            1, 1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 7
    };

    float4x4 B{
            1, 1, 1, 1,
            2, 1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 7
    };

    float4 xyzw{ 1, 1, 1, 1 };
    double4 abcd{ 1, 1, 1, 1 };

    auto res1 = A * xyzw;
    auto res2 = B * abcd;

    return 0;
}

int CrossPrecisionAdd()
{
    float4 xyzw{ 1, 1, 1, 1 };
    double4 abcd{ 1, 1, 1, 1 };

    auto res0 = xyzw + xyzw;
    auto res1 = abcd + abcd;
    auto res2 = xyzw + abcd;
    auto res3 = abcd + xyzw;

    return 0;
}

int CrossPrecisionVectorMul()
{
    float4 xyzw{ 1, 1, 1, 1 };
    double4 abcd{ 1, 2, 3, 4 };

    auto res0 = xyzw * xyzw;
    auto res1 = abcd * abcd;
    auto res2 = xyzw * abcd;
    auto res3 = abcd * xyzw;

    return 0;
}


template<typename TY, size_t rows, size_t columns, bool pad>
auto Adjugate(const Matrix<columns, rows, TY, pad>& m)
{
    
}

int float4x4Inverse()
{
    using namespace FlexKit;
    using FlexKit::operator-;

    float4x4 a{
        1, 1, 2, 2,
        0, 1, 2, 2, 
        0, 0, 4, 4, 
        0, 0, 0, 4
    };

    float4x4 a_i = Inverse(a);
    float4x4 adca_ib_i = a_i * a;

    return 0;
}

int double4x4Inverse()
{
    using namespace FlexKit;
    using FlexKit::operator-;

    double2 xy{ 0, 1 };

    float4x4 a{
        1, 0, 0, 0,
        0, 1, 0, 5,
        0, 0, 1, 10,
        0, 0, 0, 1
    };

    float4x4 a_i = Inverse(a);
    float4x4 a_i_ib = a_i * a;

    double4x4 b{
        1, 0, 0, 0,
        0, 1, 0, 5,
        0, 0, 1, 10,
        0, 0, 0, 1
    };

    double4x4 b_i = Inverse(b);
    double4x4 b_i_ib = b_i * b;

    auto t = double4x4{ a_i };
    auto res = b_i - t;

    double2 xu{ 0, 1 };
    return 0;
}

int main()
{
    double3 POS{ 10, 11, 12 };
    double4{ POS, 1 };

    double4x4 WT = double4x4::Identity();
    double3 res = (WT * double4{ POS, 1 }).xyz();

    Vect<3, double, false> v{3, 2, 1};
    constexpr size_t asdf = sizeof(v);

    Matrix<3, 3, double, false> m {
        0, 1, 2,
        3, 4, 5,
        6, 7, 8 };

    constexpr size_t sm = sizeof(m);

    const float4x4 n{
            m[0], 0,
            m[1], 0,
            m[2], 0,
            0, 0, 0, 1};

    constexpr size_t sn = sizeof(n);

    //auto res0 = Float4x42Double4x4Constructions();
    //auto res1 = Double4x4Mul();
    //auto res2 = DoubleFloat4x4Mul();
    //auto res3 = CrossPrecisionDotProducts();
    //auto res4 = CrossPrecisionAdd();
    //auto res5 = CrossPrecisionMatrixMul();
    //auto res6 = CrossPrecisionVectorMul();
    //auto res7 = float4x4Inverse();
    auto res8 = double4x4Inverse();

    return 0;
}
