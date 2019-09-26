#pragma once
#include "cryptoTools/Common/Defines.h"
#include "cryptoTools/Common/Log.h"
#include "cryptoTools/Common/BitVector.h"

#include "cryptoTools/Common/MatrixView.h"
#include <cryptoTools/Common/Matrix.h>

//#include <mutex>
#include <atomic>

//using namespace boost;
using namespace std;
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/make_shared.hpp>
#include <boost/range/iterator_range.hpp>
#include <iostream>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/pending/indirect_cmp.hpp>
#include <boost/range/irange.hpp>

#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <set> 

#include "Tools/CuckooGraph.h"
#include "PsiDefines.h"

namespace osuCrypto {

	

	//TODO: cleaning 
	inline static int forwardElim(std::vector < std::vector<bool>>& mat, std::vector<std::array<block, prty2SuperBlkSize>>& y);

	// function to calculate the values of the unknowns 
	inline static std::vector<std::array<block, prty2SuperBlkSize>> backSub(std::vector < std::vector<bool>>& mat, std::vector<std::array<block, prty2SuperBlkSize>>& y);
	// function to print matrix content at any stage 
	inline static void printMatrix(std::vector < std::vector<bool>>& mat, std::vector<std::array<block, prty2SuperBlkSize>>& y)
	{
		std::cout << mat.size() << "\n";
		std::cout << mat[0].size() << "\n";

		for (int i = 0; i < mat.size(); i++, std::cout << ("\n"))
		{
			for (int j = 0; j < mat[i].size(); j++)
				std::cout << mat[i][j] << " ";

			for (int j = 0; j < prty2SuperBlkSize; j++)
				std::cout << y[i][j] << " ";
		}
		std::cout << ("\n");
	}


	// function to get matrix content 
	inline static std::vector<std::array<block, prty2SuperBlkSize>> gaussianElimination(std::vector < std::vector<bool>>& mat, std::vector<std::array<block, prty2SuperBlkSize>>& y)
	{
		//printMatrix(mat,y);
		int N = mat.size();
		/* reduction into r.e.f. */
		int singular_flag = forwardElim(mat, y);

		/* if matrix is singular */
		if (singular_flag != -1)
		{
			std::cout << ("Singular Matrix.\n");

			/* if the RHS of equation corresponding to
			zero row is 0, * system has infinitely
			many solutions, else inconsistent*/
			/*if (memcmp((u8*)&mat[singular_flag][N], (u8*)& ZeroBlock, sizeof(block))!=0 && )
				std::cout << ("Inconsistent System.");*/


				//return ;
		}

		/* get solution to system and print it using
		backward substitution */
		return backSub(mat, y);
	}

	// function for elementary operation of swapping two rows 
	inline static void swap_row(std::vector < std::vector<bool>>& mat, std::vector<std::array<block, prty2SuperBlkSize>>& y, int i, int j)
	{
		//printf("Swapped rows %d and %d\n", i, j); 
		int M = mat[0].size();

		for (int k = 0; k < M; k++)
		{
			bool temp = mat[i][k];
			mat[i][k] = mat[j][k];
			mat[j][k] = temp;


		}
		std::array<block, prty2SuperBlkSize> temp1 = y[i];
		y[i] = y[j];
		y[j] = temp1;
	}

	// function to reduce matrix to r.e.f. 
	inline static int forwardElim(std::vector < std::vector<bool>>& mat, std::vector<std::array<block, prty2SuperBlkSize>>& y)
	{



		int N = mat.size();
		int M = mat[0].size();
		//printMatrix(mat, y);
		int colIdx = 0;
		int rowId = 0;
		while (rowId < N)
		{
			// Initialize maximum value and index for pivot 
			int i_max = rowId;
			//std::cout << "rowId " << rowId << " = 0 \n";

			bool v_max = mat[i_max][colIdx];

			/* find greater amplitude for pivot if any */
			for (int i = rowId; i < N; i++)
				if (mat[i][colIdx])
				{
					v_max = mat[i][colIdx];
					i_max = i;
					break;
				}
			/* if a prinicipal diagonal element is zero,
			* it denotes that matrix is singular, and
			* will lead to a division-by-zero later. */
			if (!v_max) //all values at colum k =0 => skip this column 
			{
				//std::cout << "diag " << colIdx << " = 0 \n";
				colIdx++;
				continue;
			}
			/* Swap the greatest value row with current row */
			if (i_max != rowId)
				swap_row(mat, y, rowId, i_max);


			for (int i = rowId + 1; i < N; i++)
			{
				/* factor f to set current row kth element to 0,
				* and subsequently remaining kth column to 0 */
				//if (mat[i][k] == 0)
				if (!mat[i][colIdx])
					continue;


				/* subtract fth multiple of corresponding kth
				row element*/
				for (int j = colIdx; j < M; j++)
					mat[i][j] = mat[i][j] ^ mat[rowId][j];// -mat[k][j];
				
				for (int j = 0; j < prty2SuperBlkSize; j++)
					y[i][j] = y[i][j] ^ y[rowId][j];

				/* filling lower triangular matrix with zeros*/
				//mat[i][k] = 0;
			}

			//printMatrix(mat, y);	 //for matrix state 
			rowId++;
		}
		//printMatrix(mat, y);		 //for matrix state 
		return -1;
	}

	// function to calculate the values of the unknowns 
	inline static std::vector<std::array<block, prty2SuperBlkSize>> backSub(std::vector < std::vector<bool>>& mat, std::vector<std::array<block, prty2SuperBlkSize>>& y)
	{
		PRNG prng(ZeroBlock);


		int N = mat.size();
		int M = mat[0].size();

		std::vector<std::array<block, prty2SuperBlkSize>>x(M); // An array to store solution 

		/* Start calculating from last equation up to the
		first */
		int previous_idx_non_zero = M;

		for (int i = N - 1; i >= 0; i--)
		{
			// find a first non-zero cell
			int idx_non_zero = -1;
			for (int j = i; j < M; j++)
				if (mat[i][j])
				{
					idx_non_zero = j;
					break;
				}

			if (idx_non_zero != -1) {

				std::array<block, prty2SuperBlkSize> sum;
				for (int k = 0; k < prty2SuperBlkSize; k++)
					sum[k] = ZeroBlock;

				for (int j = idx_non_zero + 1; j < previous_idx_non_zero; j++)
				{
					for (int k = 0; k < prty2SuperBlkSize; k++)
					{
						x[j][k] = prng.get<block>();  // chose random value for all X[from idx_non_zero to previous_idx_non_zero]
					}

					//std::cout << "x[" << j << "]= " << x[j] << "\n";
					if (mat[i][j])
						for (int k = 0; k < prty2SuperBlkSize; k++)
							sum[k] = sum[k] ^ x[j][k];

					//std::cout << "sum= " << sum << "\n";

				}

				for (int j = previous_idx_non_zero; j < M; j++)
				{
					// these x[j] have been assigned before
					if (mat[i][j])
						for (int k = 0; k < prty2SuperBlkSize; k++)
							sum[k] = sum[k] ^ x[j][k];

				//	std::cout << "sum= " << sum << "\n";
				}

				/* divide the RHS by the coefficient of the
				unknown being calculated */
				for (int k = 0; k < prty2SuperBlkSize; k++)
					x[idx_non_zero][k] = (y[i][k] ^ sum[k]);
				
				//std::cout << "xx[" << idx_non_zero << "][0]= " << x[idx_non_zero][0] << "\n";

				previous_idx_non_zero = idx_non_zero;
			}
		}

		//std::cout << "\nSolution for the system:\n";
		//for (int i = 0; i < M; i++)
		//	std::cout << x[i] << "\n";

		return x;
	}


	inline static void Cuckoo_encode(span<block> xInputs, span<std::array<block, prty2SuperBlkSize>> yInputs, std::vector<std::array<block, prty2SuperBlkSize>>& tblCuckoo, int numBin, int sigma)
	{
		std::vector<std::array<block, prty2SuperBlkSize>> R(sigma);
		tblCuckoo.resize(numBin);
		std::vector<block> functionR(xInputs.size());
		mAesFixedKey.ecbEncBlocks(xInputs.data(), xInputs.size(), functionR.data()); // use diff key for R
		
		
		for (int j = 0; j < prty2SuperBlkSize; j++)
		{
			for (int i = 0; i < tblCuckoo.size(); i++)
				tblCuckoo[i][j] = AllOneBlock;

			for (int i = 0; i < R.size(); i++)
				R[i][j] = ZeroBlock;

		}

		u64 setSize = xInputs.size();
		PRNG prng(ZeroBlock);

		MyVisitor graph;
		graph.init(setSize, 2, numBin, sigma);
		graph.buidingGraph(xInputs);
		std::cout << "\ngraph.cuckooGraph.m_edges.size(): " << graph.mCuckooGraph.m_edges.size();


		boost::depth_first_search(graph.mCuckooGraph, boost::visitor(graph));

		auto dfs_circles = graph.GetDfsCircles();
		auto dfs_visitor = graph.GetDfsVisitor();
		//auto dfs_back_edge = graph.GetDfsBackEdge();
		
		//gTimer.setTimePoint("r_Dfs");


		std::cout << "\ndfs_circles.size(): " << dfs_circles.size();
		//std::cout << "\ndfs_back_edge.size(): " << dfs_back_edge.size();
		std::cout << "\ndfs_visitor.size(): " << dfs_visitor.size();

#ifdef DEBUG_PRTY
		std::cout << "\n=============\n";
		for (int i = 0; i < dfs_circles.size(); ++i)
		{

			if (dfs_circles[i].size() > 1) //circle
			{

				for (int k = 0; k < dfs_circles[i].size() - 1; ++k)
				{
					std::cout << "tree_edge: " << dfs_circles[i][k] << " == ";
					std::cout << graph.mEdgeIdxMap_new.at({ dfs_circles[i][k].m_source,dfs_circles[i][k].m_target }) << '\n';
				}
			}
			auto k = dfs_circles[i].size() - 1;
			std::cout << "back_edge: " << dfs_circles[i][k] << " == ";
			std::cout << graph.mEdgeIdxMap_new.at({ dfs_circles[i][k].m_source,dfs_circles[i][k].m_target }) << "\n===\n\n";
		}


		for (auto it = dfs_visitor.begin(); it != dfs_visitor.end(); ++it)
		{
			std::cout << "tree_edge: " << *it << " == ";
			std::cout << graph.mEdgeIdxMap_new.at({it->m_source,it->m_target}) << "\n";

		}
#endif // DEBUG_PRTY

		std::vector < std::vector<bool>> GaussMatrix, copy_GaussMatrix;
		std::vector<std::array<block, prty2SuperBlkSize>> assocated_values, copy_assocated_values; //for test


		block equation;
		std::array<block, prty2SuperBlkSize> assocated_value;

		for (int i = 0; i < dfs_circles.size(); ++i)
		{
			equation = ZeroBlock;
			for (int k= 0; k < prty2SuperBlkSize; ++k) 
				assocated_value[k] = ZeroBlock;
			
			for (int j = 0; j < dfs_circles[i].size(); ++j) { // each circle

				//auto keyEgdeMapping = 
				(dfs_circles[i][j], graph.mNumBins);
				//auto idx_item_in_circle = graph.mEdgeIdxMap[keyEgdeMapping];
				auto idx_item_in_circle = graph.mEdgeIdxMap_new.at({ dfs_circles[i][j].m_source,dfs_circles[i][j].m_target});

				equation = equation ^ functionR[idx_item_in_circle];
				
				for (int k = 0; k < prty2SuperBlkSize; ++k) 
					assocated_value[k] = assocated_value[k] ^ yInputs[idx_item_in_circle][k];
			}
			BitVector coeff((u8*)& equation, graph.mSigma);
			auto row = bitVector2BoolVector(coeff);
			//print_BoolVector(row);
			//std::cout << coeff << "\n";
			GaussMatrix.push_back(row);
			assocated_values.push_back(assocated_value);

		}

#ifdef DEBUG_PRTY
		copy_GaussMatrix = GaussMatrix;//for test
		copy_assocated_values = assocated_values;
#endif // DEBUG_PRTY

		//Solution 

		if (GaussMatrix.size() > 0)
			R = gaussianElimination(GaussMatrix, assocated_values);

		//gTimer.setTimePoint("r_GaussMatrix");

#ifdef DEBUG_PRTY
		for (size_t i = 0; i < copy_assocated_values.size(); i++)
		{
			std::cout << copy_assocated_values[i][0] << " ==copy_assocated_values== " << assocated_values[i][0] << "\n";

		}


		for (int i = 0; i < copy_GaussMatrix.size(); i++)
		{
			std::array<block, prty2SuperBlkSize> sum;
			for (int k = 0; k < prty2SuperBlkSize; ++k) 
				sum[k]= ZeroBlock;
			
			for (u64 j = 0; j < copy_GaussMatrix[i].size(); j++)
			{
				if (copy_GaussMatrix[i][j])
					for (int k = 0; k < prty2SuperBlkSize; ++k) 
						sum[k] = sum[k] ^ R[j][k];
			}

			for (int k = 0; k < prty2SuperBlkSize; ++k) 
				if (neq(sum[k], copy_assocated_values[i][k]))
				{
					std::cout << sum[k] << " sum vs " << copy_assocated_values[i][k] << "\n";
					std::cout << "failed " << i << std::endl;
					//throw UnitTestFail();
				}

		}
#endif // DEBUG_PRTY


		//================Fill D
		//std::cout << graph.mEdgeIdxMap_new.size() << " mEdgeIdxMap_new.size().size()============\n";


		bool isRoot;
		for (auto it = dfs_visitor.begin(); it != dfs_visitor.end(); ++it)
		{
			auto edge = *it;
			auto idxItem = graph.mEdgeIdxMap_new.at({ edge.m_source,edge.m_target });

				bool isRoot = eq(tblCuckoo[edge.m_source][0], AllOneBlock);
				if (isRoot) //root
				{
					for (int k = 0; k < prty2SuperBlkSize; k++)
						tblCuckoo[edge.m_source][k] = ZeroBlock;// prng.get<block>();
					
					//std::cout << idxItem << " , ";

				}
				//compute h2
				if (eq(tblCuckoo[edge.m_target][0], AllOneBlock))
				{
					for (int k = 0; k < prty2SuperBlkSize; k++)
						tblCuckoo[edge.m_target][k] = tblCuckoo[edge.m_source][k] ^ yInputs[idxItem][k];
					
					block valueR = functionR[idxItem];

					BitVector coeff((u8*)& valueR, graph.mSigma);
					//std::cout << " \n " << coeff << " coeff  == " << idxItem << " idx ";


					for (int b = 0; b < coeff.size(); b++)
					{
						if (coeff[b])
							for (int k = 0; k < prty2SuperBlkSize; k++)
								tblCuckoo[edge.m_target][k] = tblCuckoo[edge.m_target][k] ^ R[b][k];
					}
				}
				else {
					std::cout << " \n " << idxItem << " idx ";

					cout << edge.m_target << " tblCuckoo: " << tblCuckoo[edge.m_target][0] << " already fixed \n";

				}
		}

#if 0
		//==========decode
		std::array<block, prty2SuperBlkSize> x;
		for (int i = 0; i < xInputs.size(); ++i)
		{
			auto h1 = graph.hashes1[i];
			auto h2 = graph.hashes2[i];
			
			for (int k = 0; k < prty2SuperBlkSize; k++)
				x[k] = tblCuckoo[h1][k] ^ tblCuckoo[h2][k];

			BitVector coeff((u8*)& functionR[i], graph.mSigma);
			//std::cout << coeff << "\n";
			for (int b = 0; b < coeff.size(); b++)
			{
				if (coeff[b])
				{
					//std::cout << coeff[b] << " coeff[b]\n";
					for (int k = 0; k < prty2SuperBlkSize; k++)
						x[k] = x[k] ^ R[b][k];
				}
			}
			for (int k = 0; k < prty2SuperBlkSize; k++)
				if (neq(x[k], yInputs[i][k]))
				{
					std::cout << i << ":" << x[k] << " decode failed vs " << yInputs[i][k] << "\n";
				}

		}

#endif
		tblCuckoo.insert(tblCuckoo.end(), R.begin(), R.end());

	}

	inline static void Cuckoo_decode(span<block> xInputs, std::vector<std::array<block, prty2SuperBlkSize>>& outputs, span<std::array<block, prty2SuperBlkSize>> tblCuckoo, u64 numBins)
	{
		std::vector<block> functionR(xInputs.size());
		std::vector<u64> hashes1(xInputs.size());
		std::vector<u64> hashes2(xInputs.size());
		outputs.resize(xInputs.size());
		u64 sigma = tblCuckoo.size() - numBins;

		std::vector<block> ciphers(xInputs.size());
		mAesFixedKey.ecbEncBlocks(xInputs.data(), xInputs.size(), ciphers.data());
		mAesFixedKey.ecbEncBlocks(xInputs.data(), xInputs.size(), functionR.data()); // use diff key for R


		for (int idxItem = 0; idxItem < xInputs.size(); idxItem++)
		{
			u64 hh1 = _mm_extract_epi64(ciphers[idxItem], 0);
			u64 hh2 = _mm_extract_epi64(ciphers[idxItem], 1);

			hashes1[idxItem] = hh1 % numBins; //1st 64 bits for finding bin location
			hashes2[idxItem] = hh2 % numBins; //2nd 64 bits for finding alter bin location
		}

		//==========decode
		for (int i = 0; i < xInputs.size(); ++i)
		{
			auto h1 = hashes1[i];
			auto h2 = hashes2[i];
			for (int k = 0; k < prty2SuperBlkSize; k++)
				outputs[i][k] = tblCuckoo[h1][k] ^ tblCuckoo[h2][k];

			BitVector coeff((u8*)& functionR[i], sigma);
			//std::cout << coeff << "\n";
			for (int b = 0; b < coeff.size(); b++)
			{
				if (coeff[b])
				{
					//std::cout << coeff[b] << " coeff[b]\n";
					for (int k = 0; k < prty2SuperBlkSize; k++)
						outputs[i][k] = outputs[i][k] ^ tblCuckoo[numBins+b][k];
				}
			}
			

		}


	}

	inline static void Cuckoo_decode(span<block> xInputs, Matrix<block>& yOutputs, Matrix<block>& tblCuckoo, u64 numBins, u64 sigma)
	{
#if 1
		std::vector<block> functionR(xInputs.size());
		yOutputs.resize(xInputs.size(), tblCuckoo.stride()); // row, col]

		//std::cout << yOutputs.size() << " yOutputs " << yOutputs.stride() << "\n";
		//std::cout << tblCuckoo.rows() << " tblCuckoo.rows() " << tblCuckoo.stride() << "\n";
		//std::cout << sigma << " sigma " << numBins << " numBins\n";

		
		std::vector<block> ciphers(xInputs.size());
		mAesFixedKey.ecbEncBlocks(xInputs.data(), xInputs.size(), ciphers.data());
		mAesFixedKey.ecbEncBlocks(xInputs.data(), xInputs.size(), functionR.data()); // use diff key for R

		for (int i = 0; i< xInputs.size(); i++)
		{
			u64 hh1 = _mm_extract_epi64(ciphers[i], 0);
			u64 hh2 = _mm_extract_epi64(ciphers[i], 1);

			auto h1 = hh1 % numBins; //1st 64 bits for finding bin location
			auto h2 = hh2 % numBins; //2nd 64 bits for finding alter bin location
		
			block* lVal_h1 = tblCuckoo.data() + tblCuckoo.stride() * h1;
			block* lVal_h2 = tblCuckoo.data() + tblCuckoo.stride() * h2;

			block* cVal = yOutputs.data() + yOutputs.stride() * i;

			for (u64 j = 0; j < tblCuckoo.stride(); ++j) // for all colums
			{
				cVal[j] = lVal_h1[j] ^ lVal_h2[j];
			}

			BitVector coeff((u8*)& functionR[i], sigma);
			//std::cout << sigma << "  sigma\n";
			for (int b = 0; b < sigma; b++)
			{
				if (coeff[b])
				{
					//std::cout << coeff[b] << " coeff[b]\n";
					block* lVal_b = tblCuckoo.data() + tblCuckoo.stride() * (numBins + b);
					for (u64 j = 0; j < tblCuckoo.stride(); ++j)
					{
						//std::cout <<b <<" == " << i << " = " << j  <<  " row col\n";

						cVal[j] = cVal[j] ^ lVal_b[j];
					}
				}
			}
		}

#endif
	}




}