#include				"Structures.h"
#include				"Editor Header.h"
#include				"Matrix.h"
#include				"Vector.h"
#include				"Clipping.h"

//CONSTANTES:

#define					boundrySlop				0.1
#define					kNbFields				16

//MACROS:

#define CLIPX(p1, p2, p3, xBound) Clip_TQAVTexture(p1, p2, p3, (p2->x - xBound) / (p2->x - p1->x))
#define CLIPY(p1, p2, p3, yBound) Clip_TQAVTexture(p1, p2, p3, (p2->y - yBound) / (p2->y - p1->y))
#define CLIPZ(p1, p2, p3, zBound) Clip_TQAVTexture(p1, p2, p3, (p2->z - zBound) / (p2->z - p1->z))
#define Moderate(a,b) (a + (b - a) * weight)

#define CheckX(x) ((x > 0.0) && (x < state->viewWidth))
#define CheckY(y) ((y > 0.0) && (y < state->viewHeight))
#define Check_Vertex(v) (CheckX(v.x) && CheckY(v.y))

//ROUTINES:

void Draw_TriTexture(State *state, TQAVTexture* v1, TQAVTexture* v2, TQAVTexture* v3)
{
	if(filled) {
		if(state->textured)
		QADrawTriTexture(state->drawContext, v1, v2, v3, 0);
		else
		QADrawTriGouraud(state->drawContext, (TQAVGouraud*) v1, (TQAVGouraud*) v2, (TQAVGouraud*) v3, 0);
	}
	else {
		QADrawLine(state->drawContext, (TQAVGouraud*) v1, (TQAVGouraud*) v2);
		if(special >= 2)
		QADrawLine(state->drawContext, (TQAVGouraud*) v2, (TQAVGouraud*) v3);
		if(special >= 3)
		QADrawLine(state->drawContext, (TQAVGouraud*) v3, (TQAVGouraud*) v1);
	}
}

static void ConvertTo2D(State *state, TQAVTexture* v1, TQAVTexture* v2, TQAVTexture* v3)
{
	float iw;
	TQAVTexture * v;	
	TQAVTexture verts[3];
	long c;
	
	float pixelConversion = (state->d / state->h) * (-state->viewWidth / 2);
	float invRange = 1.0 / (state->f - state->d);
	
	verts[0] = *v1;
	verts[1] = *v2;
	verts[2] = *v3;

	for(c = 0; c < 3; c++){
		v = &verts[c];

		iw = 1.0 / v->z;
		if(!orthographic) {
			v->x = v->x * iw * pixelConversion + (state->viewWidth/2);
			v->y = v->y * iw * pixelConversion + (state->viewHeight/2);
		}
		else {
			v->x = v->x / orthographicScale * pixelConversion + (state->viewWidth/2);
			v->y = v->y / orthographicScale * pixelConversion + (state->viewHeight/2);
		}
		//v->z = (v->z * (state->f / (state->f - state->d)) + (-state->f * state->d / (state->f - state->d))) * iw;
		v->z = (v->z - state->d) * invRange;
		
		v->invW = iw;
		v->uOverW *= iw;
		v->vOverW *= iw;
	}
	
#if 0
	if(Check_Vertex(verts[0]) || Check_Vertex(verts[1]) || Check_Vertex(verts[2]))	
#endif
	Clip_LeftBound(state, &verts[0], &verts[1], &verts[2]);
}

static void Clip_TQAVTexture(TQAVTexture* v1, TQAVTexture* v2, TQAVTexture* v3, float weight)
{
#if 0
	float weight2 = 1.0 - weight;
	
	v3->x = v2->x + (v1->x - v2->x) * weight;
	v3->y = v2->y + (v1->y - v2->y) * weight;
	v3->z = v2->z + (v1->z - v2->z) * weight;
	
	v3->invW = v1->invW * weight + v2->invW * weight2;
	
	v3->r = v1->r * weight + v2->r * weight2;
	v3->g = v1->g * weight + v2->g * weight2;
	v3->b = v1->b * weight + v2->b * weight2;
	v3->a = v1->a * weight + v2->a * weight2;
	
	v3->uOverW = v1->uOverW * weight + v2->uOverW * weight2;
	v3->vOverW = v1->vOverW * weight + v2->vOverW * weight2;
	
	v3->kd_r = v1->kd_r * weight + v2->kd_r * weight2;
	v3->kd_g = v1->kd_g * weight + v2->kd_g * weight2;
	v3->kd_b = v1->kd_b * weight + v2->kd_b * weight2;
	
	v3->ks_r = v1->ks_r * weight + v2->ks_r * weight2;
	v3->ks_g = v1->ks_g * weight + v2->ks_g * weight2;
	v3->ks_b = v1->ks_b * weight + v2->ks_b * weight2;
#else
	float *f1, *f2, *f3;
	unsigned long loop = kNbFields;
	
	f1 = (float*) v1;
	f2 = (float*) v2;
	f3 = (float*) v3;
	
	do {
		*f3 = Moderate(*f2, *f1);
		++f1;
		++f2;
		++f3;
	} while(--loop);
#endif
}

void Clip_Z(State *state, TQAVTexture* v1, TQAVTexture* v2, TQAVTexture* v3)
{
	TQAVTexture ta, tb;
	float bound = state->d + boundrySlop;
	
	if(v1->z < bound){
		if(v2->z < bound){
			if(v3->z < bound){
				// all points are out of bounds, draw nothing
				return;
			} else {
				// 1 & 2 are out
				CLIPZ(v1, v3, &ta, bound);
				CLIPZ(v2, v3, &tb, bound);
				ConvertTo2D(state, &ta, &tb, v3);
			}
		} else {
			if(v3->z < bound){
				// 1 & 3 are out
				CLIPZ(v1, v2, &ta, bound);
				CLIPZ(v3, v2, &tb, bound);
				ConvertTo2D(state, &ta, &tb, v2);
			} else {
				// 1 is out
				CLIPZ(v1, v2, &ta, bound);
				CLIPZ(v1, v3, &tb, bound);
				ConvertTo2D(state, &ta, v2, v3);
				ConvertTo2D(state, &tb, &ta, v3);
			}

		}
	} else {
		if(v2->z < bound){
			if(v3->z < bound){
				// 2 & 3 are out
				CLIPZ(v2, v1, &ta, bound);
				CLIPZ(v3, v1, &tb, bound);
				ConvertTo2D(state, &ta, &tb, v1);
				return;
			} else {
				// 2 is out
				CLIPZ(v2, v1, &ta, bound);
				CLIPZ(v2, v3, &tb, bound);
				ConvertTo2D(state, &ta, v1, v3);
				ConvertTo2D(state, &tb, &ta, v3);
			}
		} else {
			if(v3->z < bound){
				// 3 is out
				CLIPZ(v3, v1, &ta, bound);
				CLIPZ(v3, v2, &tb, bound);
				ConvertTo2D(state, &ta, v1, v2);
				ConvertTo2D(state, &tb, &ta, v2);
			} else {
				// all are in
				ConvertTo2D(state, v1, v2, v3);
			}
		}
	}
}

void Clip_LeftBound(State *state, TQAVTexture* v1, TQAVTexture* v2, TQAVTexture* v3)
{
	TQAVTexture ta, tb;
	float bound = 0 + boundrySlop;
	
	if(v1->x < bound){
		if(v2->x < bound){
			if(v3->x < bound){
				// all points are out of bounds, draw nothing
				return;
			} else {
				// 1 & 2 are out
				CLIPX(v1, v3, &ta, bound);
				CLIPX(v2, v3, &tb, bound);
				Clip_RightBound(state, &ta, &tb, v3);
			}
		} else {
			if(v3->x < bound){
				// 1 & 3 are out
				CLIPX(v1, v2, &ta, bound);
				CLIPX(v3, v2, &tb, bound);
				Clip_RightBound(state, &ta, &tb, v2);
			} else {
				// 1 is out
				CLIPX(v1, v2, &ta, bound);
				CLIPX(v1, v3, &tb, bound);
				Clip_RightBound(state, &ta, v2, v3);
				Clip_RightBound(state, &tb, &ta, v3);
			}

		}
	} else {
		if(v2->x < bound){
			if(v3->x < bound){
				// 2 & 3 are out
				CLIPX(v2, v1, &ta, bound);
				CLIPX(v3, v1, &tb, bound);
				Clip_RightBound(state, &ta, &tb, v1);
				return;
			} else {
				// 2 is out
				CLIPX(v2, v1, &ta, bound);
				CLIPX(v2, v3, &tb, bound);
				Clip_RightBound(state, &ta, v1, v3);
				Clip_RightBound(state, &tb, &ta, v3);
			}
		} else {
			if(v3->x < bound){
				// 3 is out
				CLIPX(v3, v1, &ta, bound);
				CLIPX(v3, v2, &tb, bound);
				Clip_RightBound(state, &ta, v1, v2);
				Clip_RightBound(state, &tb, &ta, v2);
				
			} else {
				// all are in
				Clip_RightBound(state, v1, v2, v3);
			}
		}
	}
}

void Clip_RightBound(State *state, TQAVTexture* v1, TQAVTexture* v2, TQAVTexture* v3)
{
	TQAVTexture ta, tb;
	float bound = state->viewWidth - boundrySlop;
	
	if(v1->x > bound){
		if(v2->x > bound){
			if(v3->x > bound){
				// all points are out of bounds, draw nothing
				return;
			} else {
				// 1 & 2 are out
				CLIPX(v1, v3, &ta, bound);
				CLIPX(v2, v3, &tb, bound);
				Clip_TopBound(state, &ta, &tb, v3);
			}
		} else {
			if(v3->x > bound){
				// 1 & 3 are out
				CLIPX(v1, v2, &ta, bound);
				CLIPX(v3, v2, &tb, bound);
				Clip_TopBound(state, &ta, &tb, v2);
			} else {
				// 1 is out
				CLIPX(v1, v2, &ta, bound);
				CLIPX(v1, v3, &tb, bound);
				Clip_TopBound(state, &ta, v2, v3);
				Clip_TopBound(state, &tb, &ta, v3);
			}

		}
	} else {
		if(v2->x > bound){
			if(v3->x > bound){
				// 2 & 3 are out
				CLIPX(v2, v1, &ta, bound);
				CLIPX(v3, v1, &tb, bound);
				Clip_TopBound(state, &ta, &tb, v1);
				return;
			} else {
				// 2 is out
				CLIPX(v2, v1, &ta, bound);
				CLIPX(v2, v3, &tb, bound);
				Clip_TopBound(state, &ta, v1, v3);
				Clip_TopBound(state, &tb, &ta, v3);
			}
		} else {
			if(v3->x > bound){
				// 3 is out
				CLIPX(v3, v1, &ta, bound);
				CLIPX(v3, v2, &tb, bound);
				Clip_TopBound(state, &ta, v1, v2);
				Clip_TopBound(state, &tb, &ta, v2);
			} else {
				// all are in
				Clip_TopBound(state, v1, v2, v3);
			}
		}
	}
}

void Clip_TopBound(State *state, TQAVTexture* v1, TQAVTexture* v2, TQAVTexture* v3)
{
	TQAVTexture ta, tb;
	float bound = 0 + boundrySlop;
	
	if(v1->y < bound){
		if(v2->y < bound){
			if(v3->y < bound){
				// all points are out of bounds, draw nothing
				return;
			} else {
				// 1 & 2 are out
				CLIPY(v1, v3, &ta, bound);
				CLIPY(v2, v3, &tb, bound);
				Clip_BottomBound(state, &ta, &tb, v3);
			}
		} else {
			if(v3->y < bound){
				// 1 & 3 are out
				CLIPY(v1, v2, &ta, bound);
				CLIPY(v3, v2, &tb, bound);
				Clip_BottomBound(state, &ta, &tb, v2);
			} else {
				// 1 is out
				CLIPY(v1, v2, &ta, bound);
				CLIPY(v1, v3, &tb, bound);
				Clip_BottomBound(state, &ta, v2, v3);
				Clip_BottomBound(state, &tb, &ta, v3);
			}

		}
	} else {
		if(v2->y < bound){
			if(v3->y < bound){
				// 2 & 3 are out
				CLIPY(v2, v1, &ta, bound);
				CLIPY(v3, v1, &tb, bound);
				Clip_BottomBound(state, &ta, &tb, v1);
				return;
			} else {
				// 2 is out
				CLIPY(v2, v1, &ta, bound);
				CLIPY(v2, v3, &tb, bound);
				Clip_BottomBound(state, &ta, v1, v3);
				Clip_BottomBound(state, &tb, &ta, v3);
			}
		} else {
			if(v3->y < bound){
				// 3 is out
				CLIPY(v3, v1, &ta, bound);
				CLIPY(v3, v2, &tb, bound);
				Clip_BottomBound(state, &ta, v1, v2);
				Clip_BottomBound(state, &tb, &ta, v2);
			} else {
				// all are in
				Clip_BottomBound(state, v1, v2, v3);
			}
		}
	}
}

void Clip_BottomBound(State *state, TQAVTexture* v1, TQAVTexture* v2, TQAVTexture* v3)
{
	TQAVTexture ta, tb;
	float bound = state->viewHeight - boundrySlop;
	
	if(v1->y > bound){
		if(v2->y > bound){
			if(v3->y > bound){
				// all points are out of bounds, draw nothing
				return;
			} else {
				// 1 & 2 are out
				CLIPY(v1, v3, &ta, bound);
				CLIPY(v2, v3, &tb, bound);
				Draw_TriTexture(state, &ta, &tb, v3);
			}
		} else {
			if(v3->y > bound){
				// 1 & 3 are out
				CLIPY(v1, v2, &ta, bound);
				CLIPY(v3, v2, &tb, bound);
				Draw_TriTexture(state, &ta, &tb, v2);
			} else {
				// 1 is out
				CLIPY(v1, v2, &ta, bound);
				CLIPY(v1, v3, &tb, bound);
				Draw_TriTexture(state, &ta, v2, v3);
				Draw_TriTexture(state, &tb, &ta, v3);
			}

		}
	} else {
		if(v2->y > bound){
			if(v3->y > bound){
				// 2 & 3 are out
				CLIPY(v2, v1, &ta, bound);
				CLIPY(v3, v1, &tb, bound);
				Draw_TriTexture(state, &ta, &tb, v1);
				return;
			} else {
				// 2 is out
				CLIPY(v2, v1, &ta, bound);
				CLIPY(v2, v3, &tb, bound);
				Draw_TriTexture(state, &ta, v1, v3);
				Draw_TriTexture(state, &tb, &ta, v3);
			}
		} else {
			if(v3->y > bound){
				// 3 is out
				CLIPY(v3, v1, &ta, bound);
				CLIPY(v3, v2, &tb, bound);
				Draw_TriTexture(state, &ta, v1, v2);
				Draw_TriTexture(state, &tb, &ta, v2);
			} else {
				// all are in
				Draw_TriTexture(state, v1, v2, v3);
			}
		}
	}
}