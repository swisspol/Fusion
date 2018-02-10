//PROTOTYPES:

void Vector_Clear(VectorPtr v);
float Vector_DotProduct(VectorPtr v1, VectorPtr v2);
void Vector_Subtract(VectorPtr v1, VectorPtr v2, VectorPtr v3);
void Vector_Add(VectorPtr v1, VectorPtr v2, VectorPtr v3);
void Vector_Multiply(float f, VectorPtr v1, VectorPtr v2);
void Vector_CalculateReflection(VectorPtr l, VectorPtr n, VectorPtr r);
void Vector_CrossProduct(VectorPtr v1, VectorPtr v2, VectorPtr v3);
float Vector_Length(VectorPtr v1);
float Vector_Distance(const VectorPtr v1, const VectorPtr v2);
void Vector_Normalize(VectorPtr v1, VectorPtr v2);