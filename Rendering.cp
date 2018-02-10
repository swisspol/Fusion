#include				<fp.h>

#include				"Structures.h"
#include				"Editor Header.h"
#include				"Matrix.h"
#include				"Vector.h"
#include				"Clipping.h"

//PREPROCESSOR FLAGS:

#define DIFFUSE 0

//CONSTANTES:

#define					kMaxViewDistance				50.0
#define					kLength							2.0 / shape->scale
#define					kLength2						0.5 / shape->scale

//ROUTINES:

static TQATexture* GetTQATexturePtr(OSType texID)
{
	long				i;
	
	for(i = 0; i < textureCount; ++i)
	if(nameList[i] == texID)
	return textureList[i];
	
	return nil;
}
	
static void DrawShape_NoShading(StatePtr state, ShapePtr shape, MatrixPtr globalPos, MatrixPtr camera, ShapePtr shapeList[])
{
	long				x,
						c;
	Vector				v1,
						v2;
	Vector				n;
	TQAVTexture			verts[3];
	TQAVTexture*		v;
	VertexPtr			p;
	VectorPtr			r;
	Matrix				r1;
	VectorPtr			points;
	TriFacePtr			tri;
	int					saveTF;
	unsigned long		pn;
	
	Matrix_Negate(camera, &r1);
	Matrix_Cat(globalPos, &r1, &r1);
	Shape_LinkMatrix(shape, &r1);
	if(shape->flags & kFlag_RelativePos)
	Matrix_Cat(&shape->pos, &r1, &r1);
	
	if(shape->textureFilter != kDefaultTextureFilter) {
		saveTF = QAGetInt(state->drawContext, kQATag_TextureFilter);
		QASetInt(state->drawContext, kQATag_TextureFilter, shape->textureFilter);
	}
	if(shape->texture != kNoTexture) {
		QASetPtr(state->drawContext, kQATag_Texture, GetTQATexturePtr(shape->texture));
		state->textured = true;
		
		//KEEP ONLY FOR EDITOR!! --------------------------------------------------------------
		for(c = 0; c < 3; c++){
			v = &verts[c];
			v->r = 1.0;
			v->g = 1.0;
			v->b = 1.0;
		}
	}
	else
	state->textured = false;
	
	points = (Vector*) NewPtr(sizeof(Vector) * shape->pointCount);
	for(x = 0; x < shape->pointCount; x++)
	Matrix_TransformVector(&r1, &shape->pointList[x].point, &points[x]);
	
	if(shape->glow == 0.0)
	QASetInt(state->drawContext, kQATag_TextureOp, 0);
	else
	QASetInt(state->drawContext, kQATag_TextureOp, kQATextureOp_Modulate);
	
	for(x = 0; x < shape->triangleCount; x++){
		tri = &shape->triangleList[x];
		
		if(shape->backfaceCulling){
			Vector_Subtract(&points[tri->corner[1]], &points[tri->corner[0]], &v1);
			Vector_Subtract(&points[tri->corner[2]], &points[tri->corner[0]], &v2);
			Vector_CrossProduct(&v2, &v1, &n);
			if(Vector_DotProduct(&points[tri->corner[0]], &n) > 0) //if(n.z > 0) //
			continue;
		}

		for(c = 0; c < 3; c++){
			pn = tri->corner[c];
			p = &shape->pointList[pn];
			r = &points[pn];
			v = &verts[c];
			v->x = r->x;
			v->y = r->y;
			v->z = r->z;
			
			if(state->textured) {
				v->uOverW = p->u;
				v->vOverW = p->v;
				v->kd_r = v->kd_g = v->kd_b = shape->glow;
				v->ks_r = v->ks_g = v->ks_b = 0.0;
			}
			else {
				if(shape->glow == 0.0) {
					v->r = p->u;
					v->g = p->v;
					v->b = p->c;
				}
				else {
					v->r = p->u * shape->glow;
					v->g = p->v * shape->glow;
					v->b = p->c * shape->glow;
				}
			}
			v->a = shape->alpha;
		}

		if((verts[0].z > state->d) || (verts[1].z > state->d) || (verts[2].z > state->d))
			if((verts[0].z < kMaxViewDistance) && (verts[1].z < kMaxViewDistance) && (verts[2].z < kMaxViewDistance))
			Clip_Z(state, &verts[0], &verts[1], &verts[2]);
	}

	if(shape->textureFilter != kDefaultTextureFilter)
	QASetInt(state->drawContext, kQATag_TextureFilter, saveTF);
	
	DisposePtr((Ptr) points);
}

static void DrawShape_LambertShading(StatePtr state, ShapePtr shape, MatrixPtr globalPos, MatrixPtr camera, ShapePtr shapeList[])
{
	long				x,
						c;
	Vector				v1,
						v2;
	Vector				n,
						lightSource;
	TQAVTexture			verts[3];
	TQAVTexture*		v;
	VertexPtr			p;
	VectorPtr			r;
	Matrix				r1,
						localPos;
	VectorPtr			points;
	TriFacePtr			tri;
	int					saveTF;
	unsigned long		pn;
	float*				difs;
	float				difuse;
	float				shapeDifuse = shape->difuse;
	
	localPos = *globalPos;
	Shape_LinkMatrix(shape, &localPos);
	if(shape->flags & kFlag_RelativePos)
	Matrix_Cat(&shape->pos, &localPos, &localPos);
	Matrix_Negate(camera, &r1);
	Matrix_Cat(&localPos, &r1, &r1);
	
	if(shape->textureFilter != kDefaultTextureFilter) {
		saveTF = QAGetInt(state->drawContext, kQATag_TextureFilter);
		QASetInt(state->drawContext, kQATag_TextureFilter, shape->textureFilter);
	}
	if(shape->texture != kNoTexture) {
		QASetPtr(state->drawContext, kQATag_Texture, GetTQATexturePtr(shape->texture));
		state->textured = true;
		
		//KEEP ONLY FOR EDITOR!! --------------------------------------------------------------
		for(c = 0; c < 3; c++){
			v = &verts[c];
			v->r = 1.0;
			v->g = 1.0;
			v->b = 1.0;
		}
	}
	else
	state->textured = false;
	
	points = (Vector*) NewPtr(sizeof(Vector) * shape->pointCount);
	for(x = 0; x < shape->pointCount; x++)
	Matrix_TransformVector(&r1, &shape->pointList[x].point, &points[x]);
	
	difs = (float*) NewPtr(sizeof(float) * shape->normalCount);
	
	Matrix_Negate(&localPos, &r1);
	float l = Vector_Length(&state->lightVector);
	Matrix_RotateVector(&r1, &state->lightVector, &lightSource);
	Vector_Normalize(&lightSource, &lightSource);
	Vector_Multiply(l, &lightSource, &lightSource);
	
	for(x = 0; x < shape->normalCount; x++){
		difuse = Vector_DotProduct(&lightSource, &shape->normalList[x]);
#if DIFFUSE
		if(difuse < 0) {
			if(shape->backfaceCulling)
			difuse = 0;
			else
			difuse = -difuse;
		}
#else
		if(difuse < 0)
		difuse = 0;
#endif
		difs[x] = shapeDifuse * (state->ambient + difuse);
		if(difs[x] > 1.0)
		difs[x] = 1.0;
	}
	
	QASetInt(state->drawContext, kQATag_TextureOp, kQATextureOp_Modulate);
	
	for(x = 0; x < shape->triangleCount; x++){
		tri = &shape->triangleList[x];
		
		if(shape->backfaceCulling) {
			Vector_Subtract(&points[tri->corner[1]], &points[tri->corner[0]], &v1);
			Vector_Subtract(&points[tri->corner[2]], &points[tri->corner[0]], &v2);
			Vector_CrossProduct(&v2, &v1, &n);
			if(Vector_DotProduct(&points[tri->corner[0]], &n) > 0) //if(n.z > 0) //
			continue;
		}

		/*if(shape->normalMode == 1)
		difuse = difs[x];*/
		
		for(c = 0; c < 3; c++){
			pn = tri->corner[c];
			p = &shape->pointList[pn];
			r = &points[pn];
			v = &verts[c];
			
			//if(shape->normalMode == 0)
			difuse = difs[pn];
			
			v->x = r->x;
			v->y = r->y;
			v->z = r->z;
			
			if(state->textured) {
				v->uOverW = p->u;
				v->vOverW = p->v;
				v->kd_r = v->kd_g = v->kd_b = difuse;
				v->ks_r = v->ks_g = v->ks_b = 0.0;
			}
			else {
				v->r = p->u * difuse;
				v->g = p->v * difuse;
				v->b = p->c * difuse;
			}
			v->a = shape->alpha;
		}

		if((verts[0].z > state->d) || (verts[1].z > state->d) || (verts[2].z > state->d))
			if((verts[0].z < kMaxViewDistance) && (verts[1].z < kMaxViewDistance) && (verts[2].z < kMaxViewDistance))
			Clip_Z(state, &verts[0], &verts[1], &verts[2]);
	}

	if(shape->textureFilter != kDefaultTextureFilter)
	QASetInt(state->drawContext, kQATag_TextureFilter, saveTF);
	
	DisposePtr((Ptr) points);
	DisposePtr((Ptr) difs);
}

static void DrawShape_PhongShading(StatePtr state, ShapePtr shape, MatrixPtr globalPos, MatrixPtr camera, ShapePtr shapeList[])
{
	long				x,
						c;
	Vector				v1,
						v2;
	Vector				n,
						lightSource,
						toCamera,
						reflectionVector;
	TQAVTexture			verts[3];
	TQAVTexture*		v;
	VertexPtr			p;
	VectorPtr			r;
	Matrix				r1,
						localPos;
	VectorPtr			points;
	TriFacePtr			tri;
	int					saveTF;
	unsigned long		pn;
	float*				difs;
	float*				specs;
	float				difuse,
						specular;
	float				shapeDifuse = shape->difuse,
						shapeSpecular = shape->specular;
	
	localPos = *globalPos;
	Shape_LinkMatrix(shape, &localPos);
	if(shape->flags & kFlag_RelativePos)
	Matrix_Cat(&shape->pos, &localPos, &localPos);
	Matrix_Negate(camera, &r1);
	Matrix_Cat(&localPos, &r1, &r1);
	
	if(shape->textureFilter != kDefaultTextureFilter) {
		saveTF = QAGetInt(state->drawContext, kQATag_TextureFilter);
		QASetInt(state->drawContext, kQATag_TextureFilter, shape->textureFilter);
	}
	if(shape->texture != kNoTexture) {
		QASetPtr(state->drawContext, kQATag_Texture, GetTQATexturePtr(shape->texture));
		state->textured = true;
		
		//KEEP ONLY FOR EDITOR!! --------------------------------------------------------------
		for(c = 0; c < 3; c++){
			v = &verts[c];
			v->r = 1.0;
			v->g = 1.0;
			v->b = 1.0;
		}
	}
	else
	state->textured = false;
	
	points = (Vector*) NewPtr(sizeof(Vector) * shape->pointCount);
	for(x = 0; x < shape->pointCount; x++)
	Matrix_TransformVector(&r1, &shape->pointList[x].point, &points[x]);
	
	difs = (float*) NewPtr(sizeof(float) * shape->normalCount);
	specs = (float*) NewPtr(sizeof(float) * shape->normalCount);
	
	Matrix_Negate(&localPos, &r1);
	float l = Vector_Length(&state->lightVector);
	Matrix_RotateVector(&r1, &state->lightVector, &lightSource);
	Vector_Normalize(&lightSource, &lightSource);
	Vector_Multiply(l, &lightSource, &lightSource);
	
	Vector_Subtract(&camera->w, &localPos.w, &toCamera);
	Matrix_RotateVector(&r1, &toCamera, &toCamera);
	Vector_Normalize(&toCamera, &toCamera);

	for(x = 0; x < shape->normalCount; x++){
		difuse = Vector_DotProduct(&lightSource, &shape->normalList[x]);
#if DIFFUSE
		if(difuse < 0) {
			if(shape->backfaceCulling)
			difuse = 0;
			else
			difuse = -difuse;
		}
#else
		if(difuse < 0)
		difuse = 0;
#endif

		//Vector_CalculateReflection(&lightSource, &shape->normalList[x], &reflectionVector);
		Vector_Multiply(Vector_DotProduct(&lightSource, &shape->normalList[x]) * 2.0, &shape->normalList[x], &v1);
		Vector_Subtract(&v1, &lightSource, &reflectionVector);
		specular = Vector_DotProduct(&reflectionVector, &toCamera);
		if(specular < 0)
		specular = 0; //Danger
		
		difs[x] = shapeDifuse * (state->ambient + difuse);
		if(difs[x] > 1.0)
		difs[x] = 1.0;
		specs[x] = pow(specular, shapeSpecular);
	}
	
	QASetInt(state->drawContext, kQATag_TextureOp, kQATextureOp_Modulate + kQATextureOp_Highlight);
	
	for(x = 0; x < shape->triangleCount; x++){
		tri = &shape->triangleList[x];
		
		if(shape->backfaceCulling){
			Vector_Subtract(&points[tri->corner[1]], &points[tri->corner[0]], &v1);
			Vector_Subtract(&points[tri->corner[2]], &points[tri->corner[0]], &v2);
			Vector_CrossProduct(&v2, &v1, &n);
			if(Vector_DotProduct(&points[tri->corner[0]], &n) > 0) //if(n.z > 0) //
			continue;
		}

		/*if(shape->normalMode == 1) {
			difuse = difs[x];
			specular = specs[x];
		}*/
		
		for(c = 0; c < 3; c++){
			pn = tri->corner[c];
			p = &shape->pointList[pn];
			r = &points[pn];
			v = &verts[c];
			
			//if(shape->normalMode == 0) {
				difuse = difs[pn];
				specular = specs[pn];
			//}
			
			v->x = r->x;
			v->y = r->y;
			v->z = r->z;
			
			if(state->textured) {
				v->uOverW = p->u;
				v->vOverW = p->v;
				v->kd_r = v->kd_g = v->kd_b = difuse;
				v->ks_r = v->ks_g = v->ks_b = specular;
			}
			else {
				v->r = p->u * difuse + specular; if(v->r > 1.0) v->r = 1.0;
				v->g = p->v * difuse + specular; if(v->g > 1.0) v->g = 1.0;
				v->b = p->c * difuse + specular; if(v->b > 1.0) v->b = 1.0;
			}
			v->a = shape->alpha;
		}

		if((verts[0].z > state->d) || (verts[1].z > state->d) || (verts[2].z > state->d))
			if((verts[0].z < kMaxViewDistance) && (verts[1].z < kMaxViewDistance) && (verts[2].z < kMaxViewDistance))
			Clip_Z(state, &verts[0], &verts[1], &verts[2]);
	}

	if(shape->textureFilter != kDefaultTextureFilter)
	QASetInt(state->drawContext, kQATag_TextureFilter, saveTF);
	
	DisposePtr((Ptr) points);
	DisposePtr((Ptr) difs);
	DisposePtr((Ptr) specs);
}
	
static void Fill_TQAVTexture(VectorPtr source, TQAVTexture* dest)
{
	dest->x = source->x;
	dest->y = source->y;
	dest->z = source->z;
	dest->r = 0.0;
	dest->g = 1.0;
	dest->b = 0.0;
	dest->a = 1.0;
}

static void DrawShape_Selected(StatePtr state, ShapePtr shape, MatrixPtr globalPos, MatrixPtr camera, ShapePtr shapeList[])
{
	long				x;
	TQAVTexture			verts[3];
	Matrix				r1;
	VectorPtr			points;
	Boolean			oldFilled;
	
	Matrix_Negate(camera, &r1);
	Matrix_Cat(globalPos, &r1, &r1);
	Shape_LinkMatrix(shape, &r1);
	if(shape->flags & kFlag_RelativePos)
	Matrix_Cat(&shape->pos, &r1, &r1);
	
	state->textured = false;
	special = 2;
	oldFilled = filled;
	filled = false;
	
	points = (Vector*) NewPtr(sizeof(Vector) * kBBSize);
	for(x = 0; x < kBBSize; x++) {
#if 0
		Vector_Multiply(1.1, &shape->boundingBox[x], &points[x]);
		Matrix_TransformVector(&r1, &points[x], &points[x]);
#else
		Matrix_TransformVector(&r1, &shape->boundingBox[x], &points[x]);
#endif
	}
			
	Fill_TQAVTexture(&points[0], &verts[0]);
	Fill_TQAVTexture(&points[1], &verts[1]);
	Fill_TQAVTexture(&points[2], &verts[2]);
	//if((verts[0].z < kMaxViewDistance) && (verts[1].z < kMaxViewDistance) && (verts[2].z < kMaxViewDistance))
	Clip_Z(state, &verts[0], &verts[1], &verts[2]);
	
	Fill_TQAVTexture(&points[2], &verts[0]);
	Fill_TQAVTexture(&points[3], &verts[1]);
	Fill_TQAVTexture(&points[0], &verts[2]);
	//if((verts[0].z < kMaxViewDistance) && (verts[1].z < kMaxViewDistance) && (verts[2].z < kMaxViewDistance))
	Clip_Z(state, &verts[0], &verts[1], &verts[2]);
	
	Fill_TQAVTexture(&points[4], &verts[0]);
	Fill_TQAVTexture(&points[5], &verts[1]);
	Fill_TQAVTexture(&points[6], &verts[2]);
	//if((verts[0].z < kMaxViewDistance) && (verts[1].z < kMaxViewDistance) && (verts[2].z < kMaxViewDistance))
	Clip_Z(state, &verts[0], &verts[1], &verts[2]);
	
	Fill_TQAVTexture(&points[6], &verts[0]);
	Fill_TQAVTexture(&points[7], &verts[1]);
	Fill_TQAVTexture(&points[4], &verts[2]);
	//if((verts[0].z < kMaxViewDistance) && (verts[1].z < kMaxViewDistance) && (verts[2].z < kMaxViewDistance))
	Clip_Z(state, &verts[0], &verts[1], &verts[2]);
	
	special = 1;
	
	Fill_TQAVTexture(&points[0], &verts[0]);
	Fill_TQAVTexture(&points[4], &verts[1]);
	Fill_TQAVTexture(&points[0], &verts[2]);
	//if((verts[0].z < kMaxViewDistance) && (verts[1].z < kMaxViewDistance))
	Clip_Z(state, &verts[0], &verts[1], &verts[2]);
	
	Fill_TQAVTexture(&points[1], &verts[0]);
	Fill_TQAVTexture(&points[5], &verts[1]);
	Fill_TQAVTexture(&points[1], &verts[2]);
	//if((verts[0].z < kMaxViewDistance) && (verts[1].z < kMaxViewDistance))
	Clip_Z(state, &verts[0], &verts[1], &verts[2]);
	
	Fill_TQAVTexture(&points[2], &verts[0]);
	Fill_TQAVTexture(&points[6], &verts[1]);
	Fill_TQAVTexture(&points[2], &verts[2]);
	//if((verts[0].z < kMaxViewDistance) && (verts[1].z < kMaxViewDistance))
	Clip_Z(state, &verts[0], &verts[1], &verts[2]);
	
	Fill_TQAVTexture(&points[3], &verts[0]);
	Fill_TQAVTexture(&points[7], &verts[1]);
	Fill_TQAVTexture(&points[3], &verts[2]);
	//if((verts[0].z < kMaxViewDistance) && (verts[1].z < kMaxViewDistance))
	Clip_Z(state, &verts[0], &verts[1], &verts[2]);
	
	DisposePtr((Ptr) points);
	special = 3;
	filled = oldFilled;
}

static void Fill_TQAVTexture2(VectorPtr source, TQAVTexture* dest)
{
	dest->x = source->x;
	dest->y = source->y;
	dest->z = source->z;
	dest->r = 0.0;
	dest->g = 0.0;
	dest->b = 1.0;
	dest->a = 1.0;
}

static void DrawShape_Normals(StatePtr state, ShapePtr shape, MatrixPtr globalPos, MatrixPtr camera, ShapePtr shapeList[])
{
	long				x,
						c;
	Vector				v1,
						v2;
	Vector				n,
						s;
	TQAVTexture			verts[3];
	TQAVTexture*		v;
	VectorPtr			r;
	Matrix				r1;
	VectorPtr			points,
						normalPoints;
	TriFacePtr			tri;
	unsigned long		pn;
	Boolean			oldFilled;
	
	Matrix_Negate(camera, &r1);
	Matrix_Cat(globalPos, &r1, &r1);
	Shape_LinkMatrix(shape, &r1);
	if(shape->flags & kFlag_RelativePos)
	Matrix_Cat(&shape->pos, &r1, &r1);
	
	state->textured = false;
	special = 1;
	oldFilled = filled;
	filled = false;
	
	points = (Vector*) NewPtr(sizeof(Vector) * shape->pointCount);
	for(x = 0; x < shape->pointCount; x++)
	Matrix_TransformVector(&r1, &shape->pointList[x].point, &points[x]);
	
	normalPoints = (Vector*) NewPtr(sizeof(Vector) * shape->normalCount);
	for(x = 0; x < shape->normalCount; x++) {
		Vector_Multiply(kLength2, &shape->normalList[x], &normalPoints[x]);
		Matrix_RotateVector(&r1, &normalPoints[x], &normalPoints[x]);
	}
	
	for(x = 0; x < shape->triangleCount; x++){
		tri = &shape->triangleList[x];
		
		if(shape->backfaceCulling){
			Vector_Subtract(&points[tri->corner[1]], &points[tri->corner[0]], &v1);
			Vector_Subtract(&points[tri->corner[2]], &points[tri->corner[0]], &v2);
			Vector_CrossProduct(&v2, &v1, &n);
			if(Vector_DotProduct(&points[tri->corner[0]], &n) > 0) //if(n.z > 0) //
			continue;
		}
		
		if(shape->normalMode == 0) {
			for(c = 0; c < 3; c++){
				pn = tri->corner[c];
				s = points[pn];
				Vector_Add(&s, &normalPoints[pn], &n);	
				
				Fill_TQAVTexture2(&s, &verts[0]);
				Fill_TQAVTexture2(&n, &verts[1]);
				Fill_TQAVTexture2(&s, &verts[2]);
				if((verts[0].z < kMaxViewDistance) && (verts[1].z < kMaxViewDistance))
				Clip_Z(state, &verts[0], &verts[1], &verts[2]);
			}
		}
		
		/*if(shape->normalMode == 1) {
			Vector_Clear(&s);
			for(c = 0; c < 3; c++) {
				pn = tri->corner[c];
				Vector_Add(&s, &points[pn], &s);
			}
			s.x /= 3;
			s.y /= 3;
			s.z /= 3;
			Vector_Add(&s, &normalPoints[x], &n);
			
			Fill_TQAVTexture2(&s, &verts[0]);
			Fill_TQAVTexture2(&n, &verts[1]);
			Fill_TQAVTexture2(&s, &verts[2]);
			if((verts[0].z < kMaxViewDistance) && (verts[1].z < kMaxViewDistance))
			Clip_Z(state, &verts[0], &verts[1], &verts[2]);
		}*/
	}

	DisposePtr((Ptr) points);
	DisposePtr((Ptr) normalPoints);
	special = 3;
	filled = oldFilled;
}

static void Fill_TQAVTexture3(VectorPtr source, TQAVTexture* dest)
{
	dest->x = source->x;
	dest->y = source->y;
	dest->z = source->z;
	dest->r = 1.0;
	dest->g = 0.0;
	dest->b = 0.0;
	dest->a = 1.0;
}

static void DrawShape_Center(StatePtr state, ShapePtr shape, MatrixPtr globalPos, MatrixPtr camera, ShapePtr shapeList[])
{
	TQAVTexture			verts[3];
	Matrix				r1;
	Vector				points[2];
	Boolean			oldFilled;
	
	Matrix_Negate(camera, &r1);
	Matrix_Cat(globalPos, &r1, &r1);
	Shape_LinkMatrix(shape, &r1);
	if(shape->flags & kFlag_RelativePos)
	Matrix_Cat(&shape->pos, &r1, &r1);
	
	state->textured = false;
	special = 1;
	oldFilled = filled;
	filled = false;
	
	points[0].x = -kLength;
	points[0].y = 0.0;
	points[0].z = 0.0;
	points[1].x = kLength;
	points[1].y = 0.0;
	points[1].z = 0.0;
	Matrix_TransformVector(&r1, &points[0], &points[0]);
	Matrix_TransformVector(&r1, &points[1], &points[1]);
	Fill_TQAVTexture3(&points[0], &verts[0]);
	Fill_TQAVTexture3(&points[1], &verts[1]);
	Fill_TQAVTexture3(&points[0], &verts[2]);
	//if((verts[0].z < kMaxViewDistance) && (verts[1].z < kMaxViewDistance) && (verts[2].z < kMaxViewDistance))
	Clip_Z(state, &verts[0], &verts[1], &verts[2]);
	
	points[0].x = 0.0;
	points[0].y = -kLength;
	points[0].z = 0.0;
	points[1].x = 0.0;
	points[1].y = kLength;
	points[1].z = 0.0;
	Matrix_TransformVector(&r1, &points[0], &points[0]);
	Matrix_TransformVector(&r1, &points[1], &points[1]);
	Fill_TQAVTexture3(&points[0], &verts[0]);
	Fill_TQAVTexture3(&points[1], &verts[1]);
	Fill_TQAVTexture3(&points[0], &verts[2]);
	//if((verts[0].z < kMaxViewDistance) && (verts[1].z < kMaxViewDistance) && (verts[2].z < kMaxViewDistance))
	Clip_Z(state, &verts[0], &verts[1], &verts[2]);
	
	points[0].x = 0.0;
	points[0].y = 0.0;
	points[0].z = -kLength;
	points[1].x = 0.0;
	points[1].y = 0.0;
	points[1].z = kLength;
	Matrix_TransformVector(&r1, &points[0], &points[0]);
	Matrix_TransformVector(&r1, &points[1], &points[1]);
	Fill_TQAVTexture3(&points[0], &verts[0]);
	Fill_TQAVTexture3(&points[1], &verts[1]);
	Fill_TQAVTexture3(&points[0], &verts[2]);
	//if((verts[0].z < kMaxViewDistance) && (verts[1].z < kMaxViewDistance) && (verts[2].z < kMaxViewDistance))
	Clip_Z(state, &verts[0], &verts[1], &verts[2]);
	
	DisposePtr((Ptr) points);
	special = 3;
	filled = oldFilled;
}

void Render(StatePtr state, MatrixPtr camera)
{
	long			k;
	int				save;
		
	if(!isForeGround)
	return;
	
	QARenderStart(state->drawContext, NULL, NULL);
	
	if(!selectedOnly)
	for(k = 0; k < theObject.shapeCount; ++k) {
		if(theObject.shapeList[k]->shading == 1)
		DrawShape_LambertShading(state, theObject.shapeList[k], &theObject.pos, camera, theObject.shapeList);
		else if(theObject.shapeList[k]->shading == 2)
		DrawShape_PhongShading(state, theObject.shapeList[k], &theObject.pos, camera, theObject.shapeList);
		else
		DrawShape_NoShading(state, theObject.shapeList[k], &theObject.pos, camera, theObject.shapeList);
	}
	else {
		if(theObject.shapeList[shapeCell.v]->shading == 1)
		DrawShape_LambertShading(state, theObject.shapeList[shapeCell.v], &theObject.pos, camera, theObject.shapeList);
		else if(theObject.shapeList[shapeCell.v]->shading == 2)
		DrawShape_PhongShading(state, theObject.shapeList[shapeCell.v], &theObject.pos, camera, theObject.shapeList);
		else
		DrawShape_NoShading(state, theObject.shapeList[shapeCell.v], &theObject.pos, camera, theObject.shapeList);
	}
	
	if(theObject.shapeCount != 0) {
		DrawShape_Selected(state, theObject.shapeList[shapeCell.v], &theObject.pos, camera, theObject.shapeList);
		DrawShape_Center(state, theObject.shapeList[shapeCell.v], &theObject.pos, camera, theObject.shapeList);
		if(normals && (theObject.shapeList[shapeCell.v]->normalMode != kNoNormals)) {
			save = QAGetInt(state->drawContext, kQATag_ZFunction);
			QASetInt(state->drawContext, kQATag_ZFunction, kQAZFunction_True);
			DrawShape_Normals(state, theObject.shapeList[shapeCell.v], &theObject.pos, camera, theObject.shapeList);
			QASetInt(state->drawContext, kQATag_ZFunction, save);
		}
	}
	
	QARenderEnd(state->drawContext, NULL);
}