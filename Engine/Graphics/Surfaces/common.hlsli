static const float PI = 3.1415926535897932384626433f;

float3 accurateSRGBToLinear( in float3 sRGBCol )
{
	float3 linearRGBLo = sRGBCol / 12.92;
	float3 linearRGBHi = pow( ( sRGBCol + 0.055 ) / 1.055, 2.4 );
	float3 linearRGB = ( sRGBCol <= 0.04045 ) ? linearRGBLo : linearRGBHi;
	return linearRGB;
}

float3 F_Schlick( in float3 f0, in float f90, in float u )
{
	return f0 + ( f90 - f0 ) * pow( 1.0 - u, 5.0 );
}

float Diffuse_Disney( in float NoV, in float NoL, in float LoH, in float linearRoughness )
{
	float energyBias	= lerp( 0.0, 0.5, linearRoughness );
	float energyFactor	= lerp( 1.0, 1.0 / 1.51, linearRoughness );
	float fd90			= energyBias + 2.0 * LoH * LoH * linearRoughness;
	float3 f0			= 1.0;

	float lightScatter	= F_Schlick( f0, fd90, NoL ).r;
	float viewScatter	= F_Schlick( f0, fd90, NoV ).r;

	return ( lightScatter * viewScatter * energyFactor );
}

float V_SmithGGXCorrelated( in float NdotL, in float NdotV, float alphaG )
{
	// This is the optimize version
	float alphaG2 = alphaG * alphaG;
	// Caution : the " NdotL *" and " NdotV *" are explicitely inversed , this is not a mistake .
	float Lambda_GGXV = NdotL * sqrt( ( -NdotV * alphaG2 + NdotV ) * NdotV + alphaG2 );
	float Lambda_GGXL = NdotV * sqrt( ( -NdotL * alphaG2 + NdotL ) * NdotL + alphaG2 );
	
	return 0.5f / ( Lambda_GGXV + Lambda_GGXL );
}

float D_GGX( in float NoH, in float m )
{
	float m2 = m * m;
	float f = ( NoH * m2 - NoH ) * NoH + 1.0;

	return m2 / ( f * f );
}

float smoothDistanceAtt( in float squaredDistance, in float invSqrAttRadius )
{
	float factor = squaredDistance * invSqrAttRadius;
	float smoothFactor = saturate( 1.0 - factor * factor );

	return smoothFactor * smoothFactor;
}

float getDistanceAtt( in float3 unormalizedLightVector, in float invSqrAttRadius )
{
	float sqrDist = dot( unormalizedLightVector, unormalizedLightVector );
	float attenuation = 1.0 / max( sqrDist, 0.01 * 0.01 );
	attenuation *= smoothDistanceAtt( sqrDist, invSqrAttRadius );

	return attenuation;
}

float getAngleAtt( float3 normalizedLightVector, float3 lightDir, float lightAngleScale, float lightAngleOffset )
{
	// On the CPU
	// float lightAngleScale = 1.0 f / max (0.001f, ( cosInner - cosOuter ));
	// float lightAngleOffset = -cosOuter * angleScale ;

	float cd = dot( lightDir, normalizedLightVector );
	float attenuation = saturate( cd * lightAngleScale + lightAngleOffset );
	// smooth the transition
	attenuation *= attenuation;

	return attenuation;
}

float3 getDiffuseDominantDir( float3 N, float3 V, float NdotV, float roughness )
{
	float a = 1.02341f * roughness - 1.51174f;
	float b = -0.511705f * roughness + 0.755868f;
	float lerpFactor = saturate( ( NdotV * a + b ) * roughness );
	// The result is not normalized as we fetch in a cubemap
	return lerp( N, V, lerpFactor );
}

float3 getSpecularDominantDir( float3 N, float3 R, float roughness )
{
	float smoothness = saturate( 1 - roughness );
	float lerpFactor = smoothness * ( sqrt( smoothness ) + roughness );
	// The result is not normalized as we fetch in a cubemap
	return lerp( N, R, lerpFactor );
}

float3 evaluateIBLDiffuse( in float3 V, in float3 N, in float3 R, in float roughness, float NoV )
{
	float3 dominantN = getDiffuseDominantDir( N, V, NoV, roughness );
	float3 diffuseLighting = iblDiffuseTest.Sample( defaultSampler, dominantN ).xyz;
	float diffF = iblBRDF.SampleLevel( defaultSampler, float2( NoV, roughness ), 0 ).z;

	return diffuseLighting * diffF;
}

float linearRoughnessToMipLevel( float linearRoughness, int mipCount )
{
	return ( sqrt( linearRoughness ) * mipCount );
}

float3 evaluateIBLSpecular( float3 f0, float f90, in float3 N, in float3 R, in float linearRoughness, in float roughness, float NoV )
{
	#define DFG_TEXTURE_SIZE 512.0f
	#define DFG_MIP_COUNT 10
	
	float3 dominantR = getSpecularDominantDir( N, R, roughness );

	// Rebuild the function
	// L . D . ( f0.Gv.(1-Fc) + Gv.Fc ) . cosTheta / (4 . NdotL . NdotV )
	NoV = max( NoV, 0.5f / DFG_TEXTURE_SIZE );
	float mipLevel = linearRoughnessToMipLevel( linearRoughness, DFG_MIP_COUNT );
	float3 preLD = iblEnvTest.SampleLevel( defaultSampler, dominantR, mipLevel ).rgb;

	// Sample pre - integrate DFG
	// Fc = (1-H.L)^5
	// PreIntegratedDFG.r = Gv.(1-Fc)
	// PreIntegratedDFG.g = Gv.Fc
	float2 preDFG = iblBRDF.SampleLevel( defaultSampler, float2( NoV, roughness ), 0 ).xy;

	// LD.(f0.Gv.(1 - Fc) + Gv.Fc.f90)
	return preLD * ( f0 * preDFG.x + f90 * preDFG.y );
}

float computeSpecOcclusion( float NoV, float AO, float roughness )
{
	return saturate( pow( NoV + AO, exp2( -16.0f * roughness - 1.0f ) ) - 1.0f + AO );
}

float3 L_Generic( in float3 V, in float3 N, float3 L, in float3 albedo, in float ao, in float roughness, in float metalness, in float3 reflectance )
{
	const float3 H	= normalize( V + L );
	const float3 R	= reflect( -V, N );

	const float NoL	= saturate( dot( N, L ) );
	const float NoV	= abs( dot( N, V ) ) + 1e-5f; // an epsilon is required to avoid results close to 0
	const float LoH	= saturate( dot( L, H ) );
	const float NoH	= saturate( dot( N, H ) );

	const float linearRoughness = accurateSRGBToLinear( roughness ).x;

	float3 DI	= lerp( albedo, 0.0, metalness );
	float3 f0	= reflectance; // 0.16f * ( f0 * f0 ) * ( 1.0f - metalness ) + albedo * metalness (see opaque pixel shader stage)
	float f90	= saturate( 50.0 * dot( f0, 0.33 ) );

	float3 F	= F_Schlick( f0, f90, LoH );
	float Vis	= V_SmithGGXCorrelated( NoV, NoL, roughness );
	float D		= D_GGX( NoH, roughness );

	#define INV_PI ( 1.0 / PI )

	float3 Fr	= D * F * Vis * INV_PI; // Specular BRDF
	float Fd	= Diffuse_Disney( NoV, NoL, LoH, linearRoughness ) * INV_PI; // Diffuse BRDF

	float3 luminance = ( ( Fd * DI ) + Fr );

	// IBL Diffuse + Specular using the current preconvolved envmap
	luminance += evaluateIBLDiffuse( V, N, R, roughness, NoV ) * DI;
	luminance += evaluateIBLSpecular( f0, f90, N, R, linearRoughness, roughness, NoV ) * computeSpecOcclusion( NoV, ao, roughness );

	return luminance;
}

float3 L_Sun( float3 V, float3 N, float3 albedo, in float ao, in float roughness, in float metalness, in float3 f0 )
{
	const float3 R = reflect( -V, N );

	float r = sin( sunColorAndAngularRadius.w );
	float d = cos( sunColorAndAngularRadius.w );

	float DdotR = dot( sunDirectionAndIlluminanceInLux.xyz, R );
	float3 S = R - DdotR * sunDirectionAndIlluminanceInLux.xyz;

	float3 L = ( DdotR < d ) ? normalize( d * sunDirectionAndIlluminanceInLux.xyz + normalize( S ) * r ) : R;
	float illuminance = sunDirectionAndIlluminanceInLux.w * saturate( dot( N, sunDirectionAndIlluminanceInLux.xyz ) );

	return L_Generic( V, N, L, albedo, ao, roughness, metalness, f0 ) * ( sunColorAndAngularRadius.rgb ) * illuminance;
}


float illuminanceSphereOrDisk( float cosTheta, float sinSigmaSqr )
{
	float sinTheta = sqrt( 1.0f - cosTheta * cosTheta );
	float illuminance = 0.0f;

	if ( cosTheta * cosTheta > sinSigmaSqr ) {
		illuminance = PI * sinSigmaSqr * saturate( cosTheta );
	} else {
		float x = sqrt( 1.0f / sinSigmaSqr - 1.0f ); // For a disk this simplify to x = d / r
		float y = -x * ( cosTheta / sinTheta );
		float sinThetaSqrtY = sinTheta * sqrt( 1.0f - y * y );
		illuminance = ( cosTheta * acos( y ) - x * sinThetaSqrtY ) * sinSigmaSqr + atan( sinThetaSqrtY / x );
	}

	return max( illuminance, 0.0f );
}

float3 getSpecularDominantDirArea( float3 N, float3 R, float roughness )
{
	// Simple linear approximation
	float lerpFactor = ( 1 - roughness );

	return normalize( lerp( N, R, lerpFactor ) );
}

float Trace_plane( float3 o, float3 d, float3 planeOrigin, float3 planeNormal )
{
	return dot( planeNormal, ( planeOrigin - o ) / dot( planeNormal, d ) );
}

float3 L_AreaSphere( float3 V, float3 N, float3 WP, float3 albedo, in float ao, in float roughness, in float metalness, in float3 f0, sphereLight_t light )
{
	float3 unormalizedL = light.worldPositionRadius.xyz - WP;
	float3 L = normalize( unormalizedL );
	float sqrDist = dot( unormalizedL, unormalizedL );

	float cosTheta = clamp( dot( N, L ), -0.999, 0.999 );
	float sqrLightRadius = light.worldPositionRadius.w * light.worldPositionRadius.w;
	float sinSigmaSqr = min( sqrLightRadius / sqrDist, 0.9999f );
	
	float illuminance = illuminanceSphereOrDisk( cosTheta, sinSigmaSqr );
	float luminancePower = light.color.a / ( 4.0f * sqrt( light.worldPositionRadius.w * PI ) );

	float3 r = reflect( -V, N );
	r = getSpecularDominantDirArea( N, r, roughness );

	float3 centerToRay = dot( unormalizedL, r ) * r - unormalizedL;
	float3 closestPoint = unormalizedL + centerToRay * saturate( light.worldPositionRadius.w / length( centerToRay ) );
	L = normalize( closestPoint );

	return L_Generic( V, N, L, albedo, ao, roughness, metalness, f0 ) * ( ( light.color.rgb ) * luminancePower ) * illuminance;
}

static const float2 poissonDisk[4] = 
{
		float2( -0.94201624, -0.39906216 ),
		float2( 0.94558609, -0.76890725 ),
		float2( -0.094184101, -0.92938870 ),
		float2( 0.34495938, 0.29387760 )
};

float3 L_AreaDisk( float3 V, float3 N, float3 WP, float3 albedo, in float ao, in float roughness, in float metalness, in float3 f0, in float4 shadowCoords, diskAreaLight_t light )
{
	float3 unormalizedL = light.worldPositionRadius.xyz - WP;
	float3 L = normalize( unormalizedL );
	float sqrDist = dot( unormalizedL, unormalizedL );

	float cosTheta = dot( N, L );
	float sqrLightRadius = light.worldPositionRadius.w * light.worldPositionRadius.w;

	// Do not let the surface penetrate the light
	float sinSigmaSqr = sqrLightRadius / ( sqrLightRadius + max( sqrLightRadius, sqrDist ) );

	// Multiply by saturate ( dot ( planeNormal , -L)) to better match ground truth .
	float illuminance = illuminanceSphereOrDisk( cosTheta, sinSigmaSqr ) * saturate( dot( light.planeNormal.xyz, -L ) );
	float luminancePower = light.color.a / ( sqrt( light.worldPositionRadius.w * PI ) );

	float3 r = reflect( -V, N );
	r = getSpecularDominantDirArea( N, r, roughness );

	float t = Trace_plane( WP, r, light.worldPositionRadius.xyz, light.planeNormal.xyz );
	float3 p = WP + r * t;
	float3 centerToRay = p - light.worldPositionRadius.xyz;
	float3 closestPoint = unormalizedL + centerToRay * saturate( light.worldPositionRadius.w / length( centerToRay ) );
	L = normalize( closestPoint );

	float3 projUV = float3( 0.5 * shadowCoords.x / shadowCoords.w + 0.5f,
							1.0f - ( 0.5 * shadowCoords.y / shadowCoords.w + 0.5f ),
							shadowCoords.z / shadowCoords.w );

	float Depth = shadowMapTest.Sample( defaultSampler, projUV.xy ).x;
	float shadowSampled = 1.0f;

	for ( int i = 0; i < 4; i++ ) {
		if ( shadowMapTest.Sample( defaultSampler, projUV.xy + poissonDisk[i] / 700.0 ).x < ( projUV.z - 0.000001f ) ) {
			shadowSampled -= 0.250f;
		}
	}


	return L_Generic( V, N, L, albedo, ao, roughness, metalness, f0 ) * saturate( shadowSampled ) * ( luminancePower * ( light.color.rgb ) ) * illuminance;
}

float rectangleSolidAngle( float3 worldPos, float3 p0, float3 p1, float3 p2, float3 p3 )
{
	float3 v0 = p0 - worldPos;
	float3 v1 = p1 - worldPos;
	float3 v2 = p2 - worldPos;
	float3 v3 = p3 - worldPos;

	float3 n0 = normalize( cross( v0, v1 ) );
	float3 n1 = normalize( cross( v1, v2 ) );
	float3 n2 = normalize( cross( v2, v3 ) );
	float3 n3 = normalize( cross( v3, v0 ) );

	float g0 = acos( dot( -n0, n1 ) );
	float g1 = acos( dot( -n1, n2 ) );
	float g2 = acos( dot( -n2, n3 ) );
	float g3 = acos( dot( -n3, n0 ) );

	return g0 + g1 + g2 + g3 - 2 * PI;
}

float3 rayPlaneIntersect( in float3 rayOrigin, in float3 rayDirection, in float3 planeOrigin, in float3 planeNormal )
{
	float distance = dot( planeNormal, planeOrigin - rayOrigin ) / dot( planeNormal, rayDirection );
	return rayOrigin + rayDirection * distance;
}

float3 closestPointRect( in float3 pos, in float3 planeOrigin, in float3 left, in float3 up, in float halfWidth, in float halfHeight )
{
	float3 dir = pos - planeOrigin;
	float2 dist2D = float2( dot( dir, left ), dot( dir, up ) );
	float2 rectHalfSize = float2( halfWidth, halfHeight );
	dist2D = clamp( dist2D, -rectHalfSize, rectHalfSize );

	return planeOrigin + dist2D.x * left + dist2D.y * up;
}

float Trace_triangle( float3 o, float3 d, float3 A, float3 B, float3 C )
{
	float3 planeNormal = normalize( cross( B - A, C - B ) );
	float t = Trace_plane( o, d, A, planeNormal );
	float3 p = o + d*t;

	float3 N1 = normalize( cross( B - A, p - B ) );
	float3 N2 = normalize( cross( C - B, p - C ) );
	float3 N3 = normalize( cross( A - C, p - A ) );

	float d0 = dot( N1, N2 );
	float d1 = dot( N2, N3 );

	float threshold = 1.0f - 0.001f;
	return ( d0 > threshold && d1 > threshold ) ? 1.0f : 0.0f;
}

float Trace_rectangle( float3 o, float3 d, float3 A, float3 B, float3 C, float3 D )
{
	return max( Trace_triangle( o, d, A, B, C ), Trace_triangle( o, d, C, D, A ) );
}

float3 ClosestPointOnSegment( float3 a, float3 b, float3 c )
{
	float3 ab = b - a;
	float t = dot( c - a, ab ) / dot( ab, ab );
	return a + saturate( t ) * ab;
}

float3 ClosestPointOnLine( float3 a, float3 b, float3 c )
{
	float3 ab = b - a;
	float t = dot( c - a, ab ) / dot( ab, ab );
	return a + t * ab;
}

float3 L_AreaRectangle( float3 V, float3 N, float3 WP, float3 albedo, in float ao, in float roughness, in float metalness, in float3 f0, rectangleAreaLight_t light )
{
	float3 lightLeft = light.left.xyz;
	float3 lightUp   = light.up.xyz;
	
	float illuminance = 0.0;

	float3 unormalizedL = light.worldPositionRadius.xyz - WP; 
	float dist = length( unormalizedL );
	float3 L = unormalizedL / dist;

	float halfWidth = light.widthHeight.x * 0.5;
	float halfHeight = light.widthHeight.y * 0.5;

	float3 p0 = light.worldPositionRadius.xyz + lightLeft * -halfWidth + lightUp * halfHeight;
	float3 p1 = light.worldPositionRadius.xyz + lightLeft * -halfWidth + lightUp * -halfHeight;
	float3 p2 = light.worldPositionRadius.xyz + lightLeft * halfWidth + lightUp * -halfHeight;
	float3 p3 = light.worldPositionRadius.xyz + lightLeft * halfWidth + lightUp * halfHeight;

	float solidAngle = rectangleSolidAngle( WP, p0, p1, p2, p3 );

	if ( dot( WP - light.worldPositionRadius.xyz, light.planeNormal.xyz ) > 0 ) {
		illuminance = solidAngle * 0.2 * (
			saturate( dot( normalize( p0 - WP ), N ) ) +
			saturate( dot( normalize( p1 - WP ), N ) ) +
			saturate( dot( normalize( p2 - WP ), N ) ) +
			saturate( dot( normalize( p3 - WP ), N ) ) +
			saturate( dot( normalize( light.worldPositionRadius.xyz - WP ), N ) ) );
	}
	
	{
		float3 r = reflect( -V, N );
		r = getSpecularDominantDirArea( N, r, roughness );

		float traced = Trace_rectangle( WP, r, p0, p1, p2, p3 );

		[branch]
		if ( traced > 0 ) {
			// Trace succeeded so the light vector L is the reflection vector itself
			L = r;
		} else {
			// The trace didn't succeed, so we need to find the closest point to the ray on the rectangle

			// We find the intersection point on the plane of the rectangle
			float3 tracedPlane = WP + r * Trace_plane( WP, r, light.worldPositionRadius.xyz, light.planeNormal.xyz );

			// Then find the closest point along the edges of the rectangle (edge = segment)
			float3 PC[4] = {
				ClosestPointOnSegment( p0, p1, tracedPlane ),
				ClosestPointOnSegment( p1, p2, tracedPlane ),
				ClosestPointOnSegment( p2, p3, tracedPlane ),
				ClosestPointOnSegment( p3, p0, tracedPlane ),
			};
			float dist[4] = {
				distance( PC[0], tracedPlane ),
				distance( PC[1], tracedPlane ),
				distance( PC[2], tracedPlane ),
				distance( PC[3], tracedPlane ),
			};

			float3 min = PC[0];
			float minDist = dist[0];
			[unroll]
			for ( uint iLoop = 1; iLoop < 4; iLoop++ ) {
				if ( dist[iLoop] < minDist ) {
					minDist = dist[iLoop];
					min = PC[iLoop];
				}
			}

			L = min - WP;
		}
		L = normalize( L ); // TODO: Is it necessary?
	}

	float luminancePower = light.color.a / ( light.widthHeight.x * light.widthHeight.y * PI );

	return L_Generic( V, N, L, albedo, ao, roughness, metalness, f0 ) * ( ( light.color.rgb ) * luminancePower ) * illuminance;
}

float3 L_AreaTube( float3 V, float3 N, float3 WP, float3 albedo, in float ao, in float roughness, in float metalness, in float3 f0, rectangleAreaLight_t light )
{
	float3 lightLeft = light.left.xyz;

	float3 unormalizedL = light.worldPositionRadius.xyz - WP;
	float dist = length( unormalizedL );
	float3 L = normalize( unormalizedL / dist );
	float sqrDist = dot( unormalizedL, unormalizedL );

	const float halfWidth = light.widthHeight.x * 0.5f;

	float3 P0 = light.worldPositionRadius.xyz - lightLeft * halfWidth;
	float3 P1 = light.worldPositionRadius.xyz + lightLeft * halfWidth;

	float3 forward = normalize( ClosestPointOnLine( P0, P1, WP ) - WP );
	float3 up = cross( lightLeft, forward );

	float3 p0 = light.worldPositionRadius.xyz - lightLeft * halfWidth + light.widthHeight.y * up;
	float3 p1 = light.worldPositionRadius.xyz - lightLeft * halfWidth - light.widthHeight.y * up;
	float3 p2 = light.worldPositionRadius.xyz + lightLeft * halfWidth - light.widthHeight.y * up;
	float3 p3 = light.worldPositionRadius.xyz + lightLeft * halfWidth + light.widthHeight.y * up;

	float solidAngle = rectangleSolidAngle( WP, p0, p1, p2, p3 );

	float illuminance = solidAngle * 0.2 * (
		saturate( dot( normalize( p0 - WP ), N ) ) +
		saturate( dot( normalize( p1 - WP ), N ) ) +
		saturate( dot( normalize( p2 - WP ), N ) ) +
		saturate( dot( normalize( p3 - WP ), N ) ) +
		saturate( dot( normalize( light.worldPositionRadius.xyz - WP ), N ) ) );

	float3 spherePosition = ClosestPointOnSegment( P0, P1, WP );
	float3 sphereUnormL = spherePosition - WP;
	float3 sphereL = normalize( sphereUnormL );
	float sqrSphereDistance = dot( sphereUnormL, sphereUnormL );

	float fLightSphere = PI * saturate( dot( sphereL, N ) ) * ( ( light.widthHeight.y * light.widthHeight.y ) / sqrSphereDistance );

	illuminance += fLightSphere;

	float3 r = reflect( -V, N );
	r = getSpecularDominantDirArea( N, r, roughness );

	// First, the closest point to the ray on the segment
	float3 L0 = P0 - WP;
	float3 L1 = P1 - WP;
	float3 Ld = L1 - L0;

	float t = dot( r, L0 ) * dot( r, Ld ) - dot( L0, Ld );
	t /= dot( Ld, Ld ) - sqrt( dot( r, Ld ) );

	L = ( L0 + saturate( t ) * Ld );

	float3 centerToRay = max( float3( 0.001, 0.001, 0.001 ), dot( L, r ) * r - L );
	float3 closestPoint = L + centerToRay * saturate( light.widthHeight.y / length( centerToRay ) );
	L = normalize( closestPoint );

	float luminancePower = light.color.a / ( 2.0f * PI * light.widthHeight.x * light.widthHeight.y + 4.0f * sqrt( light.widthHeight.y * PI ) );

	return L_Generic( V, N, L, albedo, ao, roughness, metalness, f0 ) * ( ( light.color.rgb ) * luminancePower ) * illuminance;
}

// Based on The Order : 1886 SIGGRAPH course notes implementation
float adjustRoughness( in float inputRoughness, in float avgNormalLength )
{
	float adjustedRoughness = inputRoughness;

	if ( avgNormalLength < 1.0f ) {
		float avgNormLen2 = avgNormalLength * avgNormalLength;
		float kappa = ( 3 * avgNormalLength - avgNormalLength * avgNormLen2 ) / ( 1 - avgNormLen2 );
		float variance = 1.0f / ( 2.0 * kappa );

		adjustedRoughness = sqrt( inputRoughness * inputRoughness + variance );
	}

	return adjustedRoughness;
}
