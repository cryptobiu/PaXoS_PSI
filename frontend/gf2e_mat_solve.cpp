#include <linbox/linbox-config.h>
#include <iostream>

#include <linbox/ring/modular.h>
#include <linbox/matrix/sparse-matrix.h>
#include <linbox/solutions/solve.h>
#include <linbox/solutions/methods.h>

using namespace LinBox;
using namespace std;

#include "gf2e_mat_solve.h"

void initField(int fieldSize){
	typedef LinBox::NTL_GF2E Field;
	Field F(2, fieldSize);
}

int solve_api(const GF2EMatrix & A, const GF2EVector & B, GF2EVector & X, int fieldSize) {

    typedef LinBox::NTL_GF2E Field;
	Field F(2, fieldSize);

	size_t rows = A.size();
	if(0 == rows) {
		return -__LINE__;
	}
	if(B.size() != rows) {
		return -__LINE__;
	}
	size_t cols = A[0].size();
	if(0 == cols) {
		return -__LINE__;
	}
	SparseMatrix<Field> mA(F, rows, cols);
	for(size_t i = 0; i < rows; ++i) {
		for(size_t j = 0; j < cols; ++j) {
			mA.setEntry(i, j, A[i][j]);
		}
	}

	DenseVector<Field> vB(F, rows);

	DenseVector<Field>::iterator i = vB.begin();
	GF2EVector::const_iterator j = B.begin();
	while(i != vB.end() && j != B.end()) {
		*i++ = *j++;
	}

	DenseVector<Field> vX(F, cols);

	solve(vX, mA, vB, Method::SparseElimination());

	X.resize(cols);
	i = vX.begin();
	GF2EVector::iterator k = X.begin();
	while(i != vX.end() && k != X.end()) {
		*k++ = *i++;
	}

	return 0;
}

