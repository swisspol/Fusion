#include			"Structures.h"
#include			"Editor Header.h"
#include			"Matrix.h"
#include			"Vector.h"

//ROUTINES:

ScriptPtr Script_New()
{
	ScriptPtr		script;
	
	script = (ScriptPtr) NewPtrClear(sizeof(Script));
	
	script->animationCount = 0;
	script->flags = 0;
	script->length = 0;//kTimeUnit;
	script->startTime = 0;
	BlockMove("\pUntitled", script->name, sizeof(Str63));
	script->id = kNoID;
	
	return script;
}

void Script_Dispose(ScriptPtr script)
{
	long			i;
	
	if(script == nil)
	return;
	
	if(script->flags & kFlag_Running)
	Script_Stop(script);
	
	for(i = 0; i < script->animationCount; ++i)
	DisposePtr((Ptr) script->animationList[i]);
	
	DisposePtr((Ptr) script);
}

void Script_NewAnimation(ScriptPtr script, OSType shapeID)
{
	ShapePtr			shape = theObject.shapeList[shapeID];
	AnimationPtr		animation;
	long				i;
	
	if(script->animationCount == kMaxAnimations - 1)
	return;
	
	for(i = 0; i < script->animationCount; ++i)
	if(script->animationList[i]->shapeID == shape->id)
	return;
	
	script->animationList[script->animationCount] = (AnimationPtr) NewPtrClear(sizeof(Animation));
	animation = script->animationList[script->animationCount];
	++script->animationCount;
	
	animation->shapeID = shape->id;
	animation->flags = 0;
	
	animation->rotateX_Init = shape->rotateX;
	animation->rotateY_Init = shape->rotateY;
	animation->rotateZ_Init = shape->rotateZ;
	animation->position_Init = shape->pos.w;
	
	animation->eventCount = 0;
}

void Script_DisposeAnimation(ScriptPtr script, long animationID)
{
	long				i;
	
	DisposePtr((Ptr) script->animationList[animationID]);
	for(i = animationID + 1; i < script->animationCount; ++i)
	script->animationList[i - 1] = script->animationList[i];
	--script->animationCount;
}

void Script_AnimationAddEvent(ScriptPtr script, long animationID, long time)
{
	AnimationPtr		animation;
	ShapePtr			shape;
	
	animation = (AnimationPtr) NewPtrClear(sizeof(Animation) + sizeof(Event) * (script->animationList[animationID]->eventCount + 1));
	BlockMove(script->animationList[animationID], animation, GetPtrSize((Ptr) script->animationList[animationID]));
	DisposePtr((Ptr) script->animationList[animationID]);
	script->animationList[animationID] = animation;
	
	shape = Shape_GetFromID(animation->shapeID);
	animation->eventList[animation->eventCount].time = time;
	animation->eventList[animation->eventCount].rotateX = shape->rotateX;
	animation->eventList[animation->eventCount].rotateY = shape->rotateY;
	animation->eventList[animation->eventCount].rotateZ = shape->rotateZ;
	animation->eventList[animation->eventCount].position = shape->pos.w;
	++animation->eventCount;
}

void Script_AnimationDeleteEvent(ScriptPtr script, long animationID, long eventID)
{
	AnimationPtr		animation;
	
	BlockMove(&script->animationList[animationID]->eventList[eventID + 1], &script->animationList[animationID]->eventList[eventID], sizeof(Event) * (script->animationList[animationID]->eventCount - eventID - 1));
	
	animation = (AnimationPtr) NewPtrClear(sizeof(Animation) + sizeof(Event) * (script->animationList[animationID]->eventCount - 1));	
	BlockMove(script->animationList[animationID], animation, sizeof(Animation) + sizeof(Event) * (script->animationList[animationID]->eventCount - 1));
	DisposePtr((Ptr) script->animationList[animationID]);
	script->animationList[animationID] = animation;
	
	--animation->eventCount;
}

void Script_AnimationSetEvent(ScriptPtr script, long animationID, long eventID)
{
	AnimationPtr		animation = script->animationList[animationID];
	EventPtr			timeEvent = &animation->eventList[eventID];
	ShapePtr			shape = Shape_GetFromID(animation->shapeID);
		
	shape->rotateX = timeEvent->rotateX;
	shape->rotateY = timeEvent->rotateY;
	shape->rotateZ = timeEvent->rotateZ;
	shape->pos.w = timeEvent->position;
	Shape_UpdateMatrix(shape);
}
	
void Script_AnimationAddStartStatusEvents(ScriptPtr script, long time)
{
	long				animationID;
	AnimationPtr		animation;
	ShapePtr			shape;
	
	for(animationID = 0; animationID < script->animationCount; ++animationID) {
		animation = (AnimationPtr) NewPtrClear(sizeof(Animation) + sizeof(Event) * (script->animationList[animationID]->eventCount + 1));
		BlockMove(script->animationList[animationID], animation, GetPtrSize((Ptr) script->animationList[animationID]));
		DisposePtr((Ptr) script->animationList[animationID]);
		script->animationList[animationID] = animation;
	
		shape = Shape_GetFromID(animation->shapeID);
		animation->eventList[animation->eventCount].time = time;
		animation->eventList[animation->eventCount].rotateX = animation->rotateX_Init;
		animation->eventList[animation->eventCount].rotateY = animation->rotateY_Init;
		animation->eventList[animation->eventCount].rotateZ = animation->rotateZ_Init;
		animation->eventList[animation->eventCount].position = animation->position_Init;
		++animation->eventCount;
	}
}

void Script_UpdateLength(ScriptPtr script)
{
	long				length = 0,
						i,
						j;
	EventPtr			timeEvent;
	
	for(i = 0; i < script->animationCount; ++i) {
		timeEvent = script->animationList[i]->eventList;
		for(j = 0; j < script->animationList[i]->eventCount; ++j) {
			if(timeEvent->time > length)
			length = timeEvent->time;
			++timeEvent;
		}
	}
	
	script->length = length;
}

void Script_Run(ScriptPtr script)
{
	long				i;
	AnimationPtr		animation;
	EventPtr			timeEvent;
	ShapePtr			shape;
	
	if(script->flags & kFlag_Running)
	return;
	
	for(i = 0; i < script->animationCount; ++i) {
		animation = script->animationList[i];
		shape = Shape_GetFromID(animation->shapeID);
		
		if(animation->eventCount) {
			timeEvent = &animation->eventList[0];
			animation->currentEvent = 0;
			animation->startTime = 0;
			animation->endTime = timeEvent->time;
			animation->running = true;
			
			if(script->flags & kFlag_SmoothStart) {
				animation->rotateX = shape->rotateX;
				animation->rotateY = shape->rotateY;
				animation->rotateZ = shape->rotateZ;
				animation->position = shape->pos.w;
				
				animation->rotateX_d = (timeEvent->rotateX - shape->rotateX) / (float) timeEvent->time;
				animation->rotateY_d = (timeEvent->rotateY - shape->rotateY) / (float) timeEvent->time;
				animation->rotateZ_d = (timeEvent->rotateZ - shape->rotateZ) / (float) timeEvent->time;
				animation->position_d.x = (timeEvent->position.x - shape->pos.w.x) / (float) timeEvent->time;
				animation->position_d.y = (timeEvent->position.y - shape->pos.w.y) / (float) timeEvent->time;
				animation->position_d.z = (timeEvent->position.z - shape->pos.w.z) / (float) timeEvent->time;
			}
			else {
				animation->rotateX = animation->rotateX_Init;
				animation->rotateY = animation->rotateY_Init;
				animation->rotateZ = animation->rotateZ_Init;
				animation->position = animation->position_Init;
				
				animation->rotateX_d = (timeEvent->rotateX - animation->rotateX_Init) / (float) timeEvent->time;
				animation->rotateY_d = (timeEvent->rotateY - animation->rotateY_Init) / (float) timeEvent->time;
				animation->rotateZ_d = (timeEvent->rotateZ - animation->rotateZ_Init) / (float) timeEvent->time;
				animation->position_d.x = (timeEvent->position.x - animation->position_Init.x) / (float) timeEvent->time;
				animation->position_d.y = (timeEvent->position.y - animation->position_Init.y) / (float) timeEvent->time;
				animation->position_d.z = (timeEvent->position.z - animation->position_Init.z) / (float) timeEvent->time;
			}
		}
		else
		animation->running = false;
		
		shape->rotateX = animation->rotateX_Init;
		shape->rotateY = animation->rotateY_Init;
		shape->rotateZ = animation->rotateZ_Init;
		shape->pos.w = animation->position_Init;
		Shape_UpdateMatrix(shape);
	}
	
	script->flags |= kFlag_Running;
	script->startTime = clockTime;
}

void Script_Stop(ScriptPtr script)
{
	script->flags &= ~kFlag_Running;
	
	if(script->flags & kFlag_ResetOnLoop)
	Script_Reset(script);
}

void Script_Reset(ScriptPtr script)
{
	long				i;
	AnimationPtr		animation;
	ShapePtr			shape;
	
	for(i = 0; i < script->animationCount; ++i) {
		animation = script->animationList[i];
		shape = Shape_GetFromID(animation->shapeID);
		
		shape->rotateX = animation->rotateX_Init;
		shape->rotateY = animation->rotateY_Init;
		shape->rotateZ = animation->rotateZ_Init;
		shape->pos.w = animation->position_Init;
		Shape_UpdateMatrix(shape);
	}
}

static void Script_LoopNoReset(ScriptPtr script)
{
	long				i;
	AnimationPtr		animation;
	EventPtr			timeEvent,
						finalEvent;
	ShapePtr			shape;
	
	for(i = 0; i < script->animationCount; ++i) {
		animation = script->animationList[i];
		
		if(animation->eventCount) {
			shape = Shape_GetFromID(animation->shapeID);
			timeEvent = &animation->eventList[0];
			finalEvent = &animation->eventList[animation->currentEvent];
			animation->currentEvent = 0;
			animation->startTime = 0;
			animation->endTime = timeEvent->time;
			animation->running = true;
			
#if 1
			shape->rotateX = DegreesToRadians((long) RadiansToDegrees(finalEvent->rotateX) % 360);
			shape->rotateY = DegreesToRadians((long) RadiansToDegrees(finalEvent->rotateY) % 360);
			shape->rotateZ = DegreesToRadians((long) RadiansToDegrees(finalEvent->rotateZ) % 360);
			shape->pos.w = finalEvent->position;
#endif
			
			animation->rotateX = shape->rotateX;
			animation->rotateY = shape->rotateY;
			animation->rotateZ = shape->rotateZ;
			animation->position = shape->pos.w;
			
			animation->rotateX_d = (timeEvent->rotateX - shape->rotateX) / (float) timeEvent->time;
			animation->rotateY_d = (timeEvent->rotateY - shape->rotateY) / (float) timeEvent->time;
			animation->rotateZ_d = (timeEvent->rotateZ - shape->rotateZ) / (float) timeEvent->time;
			animation->position_d.x = (timeEvent->position.x - shape->pos.w.x) / (float) timeEvent->time;
			animation->position_d.y = (timeEvent->position.y - shape->pos.w.y) / (float) timeEvent->time;
			animation->position_d.z = (timeEvent->position.z - shape->pos.w.z) / (float) timeEvent->time;
		}
		else
		animation->running = false;
	}
	
	script->startTime = clockTime;
}

static void Script_LoopReset(ScriptPtr script)
{
	long				i;
	AnimationPtr		animation;
	EventPtr			timeEvent;
	ShapePtr			shape;
	
	for(i = 0; i < script->animationCount; ++i) {
		animation = script->animationList[i];
		shape = Shape_GetFromID(animation->shapeID);
		
		if(animation->eventCount) {
			timeEvent = &animation->eventList[0];
			animation->currentEvent = 0;
			animation->startTime = 0;
			animation->endTime = timeEvent->time;
			animation->running = true;
			
			animation->rotateX = animation->rotateX_Init;
			animation->rotateY = animation->rotateY_Init;
			animation->rotateZ = animation->rotateZ_Init;
			animation->position = animation->position_Init;
			
			animation->rotateX_d = (timeEvent->rotateX - animation->rotateX_Init) / (float) timeEvent->time;
			animation->rotateY_d = (timeEvent->rotateY - animation->rotateY_Init) / (float) timeEvent->time;
			animation->rotateZ_d = (timeEvent->rotateZ - animation->rotateZ_Init) / (float) timeEvent->time;
			animation->position_d.x = (timeEvent->position.x - animation->position_Init.x) / (float) timeEvent->time;
			animation->position_d.y = (timeEvent->position.y - animation->position_Init.y) / (float) timeEvent->time;
			animation->position_d.z = (timeEvent->position.z - animation->position_Init.z) / (float) timeEvent->time;
		}
		else
		animation->running = false;
		
		shape->rotateX = animation->rotateX_Init;
		shape->rotateY = animation->rotateY_Init;
		shape->rotateZ = animation->rotateZ_Init;
		shape->pos.w = animation->position_Init;
		Shape_UpdateMatrix(shape);
	}
	
	script->startTime = clockTime;
}

void Script_Running(ScriptPtr script)
{
	long				i;
	AnimationPtr		animation;
	EventPtr			timeEvent;
	ShapePtr			shape;
	long				time = clockTime - script->startTime;
	float				dTime;
	
	if(time >= script->length) {
		if(script->flags & kFlag_Loop) {
			if(script->flags & kFlag_ResetOnLoop)
			Script_LoopReset(script);
			else
			Script_LoopNoReset(script);
		}
		else
		Script_Stop(script);
		return;
	}
	
	for(i = 0; i < script->animationCount; ++i) {
		animation = script->animationList[i];
		if(!animation->running)
		continue;
		shape = Shape_GetFromID(animation->shapeID);
		
		if(time >= animation->endTime) {
			++animation->currentEvent;
			if(animation->currentEvent > animation->eventCount - 1)
			animation->running = false;
			else {
				timeEvent = &animation->eventList[animation->currentEvent];
				animation->startTime = time;
				animation->endTime = timeEvent->time;
				dTime = timeEvent->time - time;
				
				animation->rotateX = shape->rotateX;
				animation->rotateY = shape->rotateY;
				animation->rotateZ = shape->rotateZ;
				animation->position = shape->pos.w;
				
				animation->rotateX_d = (timeEvent->rotateX - shape->rotateX) / (float) dTime;
				animation->rotateY_d = (timeEvent->rotateY - shape->rotateY) / (float) dTime;
				animation->rotateZ_d = (timeEvent->rotateZ - shape->rotateZ) / (float) dTime;
				animation->position_d.x = (timeEvent->position.x - shape->pos.w.x) / (float) dTime;
				animation->position_d.y = (timeEvent->position.y - shape->pos.w.y) / (float) dTime;
				animation->position_d.z = (timeEvent->position.z - shape->pos.w.z) / (float) dTime;
			}
		}
		else {
			dTime = time - animation->startTime;
			shape->rotateX = animation->rotateX + animation->rotateX_d * dTime;
			shape->rotateY = animation->rotateY + animation->rotateY_d * dTime;
			shape->rotateZ = animation->rotateZ + animation->rotateZ_d * dTime;
			shape->pos.w.x = animation->position.x + animation->position_d.x * dTime;
			shape->pos.w.y = animation->position.y + animation->position_d.y * dTime;
			shape->pos.w.z = animation->position.z + animation->position_d.z * dTime;
			Shape_UpdateMatrix(shape);
		}
	}
}