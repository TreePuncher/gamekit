

void QuantizeMoments(out float3 b_even_q, out float3 b_odd_q, float3 b_even, float3 b_odd)
{
	const float3x3 QuantizationMatrixOdd = float3x3(
		 2.5f, -1.87499864450f,  1.26583039016f,
		-10.0f, 4.20757543111f, -1.47644882902f,
		 8.0f, -1.83257678661f,  0.71061660238f);
	
	const float3x3 QuantizationMatrixEven = float3x3(
		 4.0f,  9.0f,  -0.57759806484f,
		-4.0f, -24.0f,  4.61936647543f,
		 0.0f,  16.0f, -3.07953906655f);
	
	b_odd_q		= mul(b_odd, QuantizationMatrixOdd);
	b_even_q	= mul(b_even, QuantizationMatrixEven);
}

void QuantizeMoments(out float2 b_even_q, out float2 b_odd_q, float2 b_even, float2 b_odd)
{
	b_odd_q		= mul(b_odd, float2x2(1.5f, sqrt(3.0f) * 0.5f, -2.0f, -sqrt(3.0f) * 2.0f / 9.0f));
	b_even_q	= mul(b_even, float2x2(4.0f, 0.5f, -4.0f, 0.5f));
}


/************************************************************************************************/


void OffsetMoments(inout float3 b_even, inout float3 b_odd, float sign)
{
	b_odd += 0.5 * sign;
	b_even.z += 0.018888946f * sign;
}


/************************************************************************************************/


void OffsetMoments(inout float2 b_even, inout float2 b_odd, float sign)
{
	b_odd += 0.5 * sign;
}


/************************************************************************************************/


void GeneratePowerMoments(inout float b_0, inout float2 b_even, inout float2 b_odd, float depth, float transmittance)
{
	const float absorbance = -log(transmittance);
	const float depth_pow2 = depth * depth;
	const float depth_pow4 = depth_pow2 * depth_pow2;
	
	b_0		+= absorbance;
	b_even	+= float2(depth_pow2, depth_pow4)		* absorbance;
	b_odd	+= float2(depth, depth_pow2 * depth)	* absorbance;

	float2 b_even_new	= float2(depth_pow2, depth_pow4);
	float2 b_odd_new	= float2(depth, depth_pow2 * depth);
	float2 b_even_new_q, b_odd_new_q;

	QuantizeMoments(b_even_new_q, b_odd_new_q, b_even_new, b_odd_new);

	// Combine Moments
	b_0 += absorbance;
	b_even += b_even_new_q * absorbance;
	b_odd += b_odd_new_q * absorbance;

	// Go back to interval [0, 1]
	b_even /= b_0;
	b_odd /= b_0;
	
	OffsetMoments(b_even, b_odd, 1.0);
}


/************************************************************************************************/


float ComputeTransmittanceAtDepthFrom4PowerMoments(float b_0, float2 b_even, float2 b_odd, float depth, float bias, float overestimation, float4 bias_vector)
{
	float4 b = float4(b_odd.x, b_even.x, b_odd.y, b_even.y);
	// Bias input data to avoid artifacts
	b = lerp(b, bias_vector, bias);
	float3 z;
	z[0] = depth;

	// Compute a Cholesky factorization of the Hankel matrix B storing only non-
	// trivial entries or related products
	float L21D11=mad(-b[0],b[1],b[2]);
	float D11=mad(-b[0],b[0], b[1]);
	float InvD11=1.0f/D11;
	float L21=L21D11*InvD11;
	float SquaredDepthVariance=mad(-b[1],b[1], b[3]);
	float D22=mad(-L21D11,L21,SquaredDepthVariance);

	// Obtain a scaled inverse image of bz=(1,z[0],z[0]*z[0])^T
	float3 c=float3(1.0f,z[0],z[0]*z[0]);
	// Forward substitution to solve L*c1=bz
	c[1]-=b.x;
	c[2]-=b.y+L21*c[1];
	// Scaling to solve D*c2=c1
	c[1]*=InvD11;
	c[2]/=D22;
	// Backward substitution to solve L^T*c3=c2
	c[1]-=L21*c[2];
	c[0]-=dot(c.yz,b.xy);
	// Solve the quadratic equation c[0]+c[1]*z+c[2]*z^2 to obtain solutions 
	// z[1] and z[2]
	float InvC2=1.0f/c[2];
	float p=c[1]*InvC2;
	float q=c[0]*InvC2;
	float D=(p*p*0.25f)-q;
	float r=sqrt(D);
	z[1]=-p*0.5f-r;
	z[2]=-p*0.5f+r;
	// Compute the absorbance by summing the appropriate weights
	float3 polynomial;
	float3 weight_factor = float3(overestimation, (z[1] < z[0])?1.0f:0.0f, (z[2] < z[0])?1.0f:0.0f);
	float f0=weight_factor[0];
	float f1=weight_factor[1];
	float f2=weight_factor[2];
	float f01=(f1-f0)/(z[1]-z[0]);
	float f12=(f2-f1)/(z[2]-z[1]);
	float f012=(f12-f01)/(z[2]-z[0]);
	polynomial[0]=f012;
	polynomial[1]=polynomial[0];
	polynomial[0]=f01-polynomial[0]*z[1];
	polynomial[2]=polynomial[1];
	polynomial[1]=polynomial[0]-polynomial[1]*z[0];
	polynomial[0]=f0-polynomial[0]*z[0];
	float absorbance = polynomial[0] + dot(b.xy, polynomial.yz);;
	// Turn the normalized absorbance into transmittance
	return saturate(exp(-b_0 * absorbance));
}



