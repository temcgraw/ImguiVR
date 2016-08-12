//Lots of functions here from Inigo Quilez ()
// and syntopia...

#version 450

layout(location = 0) uniform mat4 PVM;
layout(location = 1) uniform mat4 P;
layout(location = 2) uniform mat4 Q1;
layout(location = 3) uniform mat4 Q2;
layout(location = 4) uniform float time = 0.0f;

layout(location = 5) uniform vec4 axis = vec4(0.0f);
layout(location = 6) uniform float scale = 1.0f;
layout(location = 7) uniform vec2 clip = vec2(1.0, 5.0);

layout(location = 8) uniform int mode = 0;
layout(location = 9) uniform float params[10];
layout(location = 19) uniform vec4 color[4];

float param_range(int id, float min, float max)
{
	return params[id]*(max-min) + min;
}

out vec4 fragcolor;   


in vec3 vpos;
in mat3 rotation;

vec4 raytracedcolor(vec3 rayStart, vec3 rayStop);
vec4 fog(float z, vec4 color, vec3 dir);
vec4 lighting(vec3 pos, vec3 rayDir); //phong with wrap
float softshadow( in vec3 ro, in vec3 rd, float mint, float maxt, float k );
float calcAO( in vec3 pos, in vec3 nor );
vec4 ambient_light_color(vec3 dir);
float distToShape(vec3 pos);
vec3 normal(vec3 pos);
vec3 fast_normal(vec3 pos);

float mat_id = 0.0;

void main(void)
{   
	vec4 nvpos = vec4(normalize(vpos), 1.0);
	vec4 rayStart = Q1*nvpos;
	vec4 rayStop = Q2*nvpos;
	
	//fragcolor = rayStart; //for debugging
	fragcolor = raytracedcolor(rayStart.xyz, rayStop.xyz);

}


vec4 raytracedcolor(vec3 rayStart, vec3 rayStop)
{
	vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
	float scale = clip[0];
	int MaxSamples = 500;

	vec3 rayDir = normalize(rayStop-rayStart);

	float travel = distance(rayStop, rayStart);
	float stepSize = travel/MaxSamples;
	vec3 pos = rayStart;
	vec3 step = rayDir*stepSize;
	
	for (int i=0; i < MaxSamples && travel > 0.0; ++i, pos += step, travel -= stepSize)
	{
		float dist = distToShape(pos);

		stepSize = 0.95*dist;
		step = rayDir*stepSize;
		
		if(dist<=0.001*scale)
		{
			color = lighting(pos, rayDir);
			color = fog(dot(rayDir, pos-rayStart), color, rayDir);
			return color;
		}	
	}

	return fog(clip[1], color, rayDir);
	
}


//compute lighting on the intersected surface
vec4 lighting(vec3 pos, vec3 rayDir)
{
	const float wrap = -0.2;
	//const vec3 light = vec3(-1.0/1.7, 1.0/1.7, 1.0/1.7);
	vec3 light_pos = vec3(Q2[3][0], Q2[3][1], Q2[3][2])+0.15*clip[0]*vec3(-1.0/1.7, 1.0/1.7, 1.0/1.7);
	float light_dist = length(light_pos - pos);
	vec3 light = (light_pos - pos)/light_dist;
	//vec3 n = normal(pos-0.002*rayDir);
	vec3 n = fast_normal(pos-0.002*rayDir);

	vec4 ambient_color = 0.7*ambient_light_color(n);
	//vec4 diffuse_color = mix(color[2], color[3], mat_id);
	vec4 diffuse_color = mix(color[2], color[3], smoothstep(0.4, 0.6, mat_id));
	const vec4 spec_color = 0.5*vec4(1.0, 1.0, 1.0, 1.0);
	
	vec3 v = -rayDir;
	vec3 r = reflect(-light, n);
	float ndotl = (dot(n, light) + wrap) / (1.0 + wrap);
	float ao = 0.0;//min(1.0, 0.1+calcAO(pos+0.005*n, n));

	//float shadow = softshadow( pos, light, 0.2*clip[0], 0.7f*clip[1], 2.0 );
	float shadow = 1.0;//softshadow( pos, light, 0.2*clip[0], light_dist, 5.0 );
	return ao*ambient_color + (shadow+0.1)*(diffuse_color*max(0.0, dot(n, light)) + spec_color*max(0.0, pow(dot(r, v), 15.0)));	//mostly diffuse
}

vec4 ambient_light_color(vec3 dir)
{
	return mix(color[0], color[1], smoothstep(-0.3, 0.2, dir.y));  //beach sky
}

vec4 fog(float z, vec4 color, vec3 dir)
{
   //return color;
   float f = smoothstep(0.4*clip[1], 0.9*clip[1], abs(z));
   vec4 fog_color = ambient_light_color(dir);
   return mix(color, fog_color, f);
}

float softshadow( in vec3 ro, in vec3 rd, float mint, float maxt, float k )
{
    float res = 1.0;
    for( float t=mint; t < maxt; )
    {
        float h = distToShape(ro + rd*t);
        if( h<0.001 )
            return 0.0;
        res = min( res, k*h/t );
        t += h;
    }
    return res;
}

float calcAO( in vec3 pos, in vec3 nor )
{
   float occ = 0.0;
   float sca = 1.0;
   for( int i=0; i<5; i++ )
   {
      float hr = 0.01 + 0.12*float(i)/4.0;
      vec3 aopos =  nor * hr + pos;
      float dd = distToShape( aopos );
      occ += -(dd-hr)*sca;
      sca *= 0.95;
   }
   return clamp( 1.0 - 3.0*occ, 0.0, 1.0 );    
}



float sdSphere( vec3 p, float s );
float sdTorus( vec3 p, vec2 t );
float sdBox( vec3 p, vec3 b );
float DE_julia(vec3 pos);
float DE_menger(int it, vec3 z, float Scale, vec3 Offset);

vec3 rep(vec3 pos, vec3 c)
{
	return mod(pos, c) - 0.5*c;
}

float union_(float d1, float d2)
{
	return min(d1, d2);
}

float intersection(float d1, float d2)
{
	return max(d1, d2);
}

float sub(float d1, float d2)
{
	return max(d1, -d2);
}

float smin(float a, float b, float k)
{
	float h = clamp(0.5 + 0.5*(b-a)/k, 0.0, 1.0);
	return mix(b, a, h) - k*h*(1.0-h);
}

float disp (vec3 p)
{
	return sin(p.x)*sin(p.y)*sin(p.z);
}

float disp2 (vec3 p)
{
	float d = sin(p.x)*sin(p.y)*sin(p.z);
	return d*d*d;
}

float distToShape(vec3 pos)
{

	if(mode==0)
	{
		float d = sdBox( pos+vec3(0.0, 1.5, 0.0), vec3(3.5, 0.2, 3.5) );
		d = min(d, sdTorus(pos-vec3(0.0, 1.5+10.0*params[0], 0.0), vec2(0.5+params[1], 0.1+params[2]) ));
		return d;
	}
	else if(mode==1)
	{
		vec3 pr = rotation*pos;
		float d = time*0.25;
		float d_big_sphere = sdSphere(pos, 1.0);
		float d_rep1 = sdSphere(rep(pr+vec3(0.0, -d, 0.0), vec3(2.8)), 0.5);

		float dp = disp(10.0*pos);
		mat_id = dp*0.5 + 0.5;

		return sub(d_rep1, d_big_sphere) + 0.08*dp;

	}
	else if(mode==2)//bubbles
	{
		float d = time*0.25;
		float d_rep1 = sdSphere(rep(pos+vec3(0.0, -d, 0.0), vec3(1.8)), 0.25);
		float d_rep2 = sdSphere(rep(pos+vec3(0.0, -1.5*d, 0.0), vec3(3.5)), 1.0);
		float d_rep3 = sdSphere(rep(pos+vec3(0.0, -2.25*d, 0.0), vec3(2.5)), 0.5);

		mat_id = float(d_rep1<d_rep2);
		const float k = 0.1;
		return smin(d_rep1, smin(d_rep2, d_rep3, k), k);
	}
	else if(mode==3)
	{
		float d = time*0.25;
		float d_big_sphere = sdSphere(pos, 1.0);
		float d_rep1 = sdSphere(rep(pos+vec3(0.0, -d, 0.0), vec3(1.8)), 0.25);
		float d_rep2 = sdSphere(rep(pos+vec3(0.0, -0.5*d, 0.0), vec3(3.5)), 1.0);
		float d_rep3 = sdSphere(rep(pos+vec3(0.0, -0.25*d, 0.0), vec3(2.5)), 0.5);

		float d_rep = union_(d_rep1, d_rep2);
		float d_sub = union_(d_big_sphere, d_rep3);

		float dp = disp(20.0*pos);
		mat_id = float(d_rep1<d_rep2);
		return sub(d_rep, d_sub) + 0.05*dp;

	}

	else if(mode==4)
	{
		
		float sc = 5.0*params[0];
		vec3 pr = rotation*(pos/sc+vec3(0.0, -10.0*params[1], 0.0));
		float d_big_sphere = sdSphere(pos, 1.0);
		float j = sc*DE_julia(pr);
		return sub(j, d_big_sphere);
	}

	else if(mode==5)
	{
		float sc = 5.0*params[0];
		vec3 pr = rotation*(pos/sc+vec3(0.0, -10.0*params[1], 0.0));
		float d_big_sphere = sdSphere(pos, 1.0);
		float m = sc*DE_menger(6, pr, 3.0, vec3(params[2], params[3], params[4]))-0.005;
		return sub(m, d_big_sphere);
	}
	else //boxes
	{
		pos.x += time;
		float d_rep1 = sdBox(rep(pos, vec3(2.00+params[0], 0.0, 2.00+params[1])), vec3(0.5));
		float d_rep2 = sdBox(rep(pos, vec3(1.00+params[2], 1.0, 1.00+params[3])), vec3(0.25+params[8]));
		float d_rep3 = sdBox(rep(pos, vec3(0.50+params[4], 1.0, 0.50+params[5])), vec3(0.125+params[9]));
		float d_rep4 = sdBox(rep(pos, vec3(0.25+params[6], 1.0, 0.25+params[7])), vec3(0.0625));

		return sub(sub(sub(d_rep1, d_rep2), d_rep3), d_rep4);
	}

}

vec3 normal(vec3 pos)
{
	float h = 0.002*clip[0];
	vec3 Xh = vec3(h, 0.0, 0.0);	
	vec3 Yh = vec3(0.0, h, 0.0);	
	vec3 Zh = vec3(0.0, 0.0, h);	

	return normalize(vec3(distToShape(pos+Xh)-distToShape(pos-Xh), distToShape(pos+Yh)-distToShape(pos-Yh), distToShape(pos+Zh)-distToShape(pos-Zh)));
}

vec3 fast_normal(vec3 pos)
{
	float dist0 = distToShape(pos);//0.001;
	float h = 0.002*clip[0];
	vec3 Xh = vec3(h, 0.0, 0.0);	
	vec3 Yh = vec3(0.0, h, 0.0);	
	vec3 Zh = vec3(0.0, 0.0, h);	

	return normalize(vec3(distToShape(pos+Xh)-dist0, distToShape(pos+Yh)-dist0, distToShape(pos+Zh)-dist0));
}
  
float sdSphere( vec3 p, float s )
{
	return length(p)-s;
}

float sdTorus( vec3 p, vec2 t )
{
  vec2 q = vec2(length(p.xz)-t.x,p.y);
  return length(q)-t.y;
}

float sdBox( vec3 p, vec3 b )
{
	vec3 d = abs(p) - b;
	return min(max(d.x,max(d.y,d.z)),0.0) +
	length(max(d, 0.0));
}


float DE_julia(vec3 pos)
{
	vec4 c = vec4(params[2], params[3], params[4], params[5]);

	vec4 z = vec4(pos, 0.0);
	vec4 nz;
	float md2 = 1.0;
	float mz2 = dot(z,z);

	for(int i=0;i<18;i++)
	{
		// |dz|^2 -> 4*|dz|^2
		md2 *= 4.0*mz2;
		// z -> z2 + c
		nz.x = z.x*z.x-dot(z.yzw,z.yzw);
		nz.yzw = 2.0*z.x*z.yzw;
		z = nz + c;

		mz2 = dot(z,z);

		if(mz2>12.0) //Bailout
		{
			break;
		}
	}

	return 0.25*sqrt(mz2/md2)*log(mz2);
}


float DE_menger(int it, vec3 z, float Scale, vec3 Offset)
{
	float d = 1.0e8;
	for (int n = 0; n < it; n++)
	{
      
      //z.xy = rotate2(z.xy, 0.0);
      //z.yz = rotate2(z.yz, 0.0);
      //z.xz = rotate2(z.xz, 0.0);

		z = abs(z);
		if (z.x<z.y){ z.xy = z.yx;}
		if (z.x<z.z){ z.xz = z.zx;}
		if (z.y<z.z){ z.yz = z.zy;}
		z = Scale*z-Offset*(Scale-1.0);
		if(z.z < -0.5*Offset.z*(Scale-1.0))  z.z += Offset.z*(Scale-1.0);

		d = min(d, length(z) * pow(Scale, float(-n)-1.0));
      if(d>1.5) break;
	}
	
	return d;
}










