#pragma once

#include <vector>
#include <NTL/GF2E.h>

typedef std::vector< NTL::GF2E > GF2EVector;
typedef std::vector< GF2EVector > GF2EMatrix;

void initField(int fieldSize);
int solve_api(const GF2EMatrix & A, const GF2EVector & B, GF2EVector & X, int fieldSize);

