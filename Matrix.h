//PROTOTYPES:

void Matrix_Clear(MatrixPtr m1);
void Matrix_Cat(MatrixPtr m1, MatrixPtr m2, MatrixPtr m3);
void Matrix_RotateByMatrix(MatrixPtr m1, MatrixPtr m2);
void Matrix_TransformVector(MatrixPtr m1, Vector* v1, Vector* v2);
void Matrix_RotateVector(MatrixPtr m1, Vector* v1, Vector* v2);
void Matrix_SetRotateX(float theta, MatrixPtr m1);
void Matrix_SetRotateY(float theta, MatrixPtr m1);
void Matrix_SetRotateZ(float theta, MatrixPtr m1);
void Matrix_Negate(MatrixPtr m1, MatrixPtr m2);
void Matrix_ScaleLocal(MatrixPtr m1, float scaleFactor, MatrixPtr m2);