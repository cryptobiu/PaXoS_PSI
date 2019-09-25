#pragma once
#include "cryptoTools/Common/Defines.h"
#include "cryptoTools/Common/Log.h"
#include "cryptoTools/Common/BitVector.h"

#include "cryptoTools/Common/MatrixView.h"
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


namespace osuCrypto
{
	
	typedef boost::adjacency_list<  // adjacency_list is a template depending on :
		boost::vecS,               //  The container used for egdes : here, std::list.
		boost::vecS,                //  The container used for vertices: here, std::vector.
		boost::undirectedS,           //  directed or undirected edges ?.
		int,                     //  The type that describes a Vertex.
		string                        //  The type that describes an Edge
	>ccGraph;


	typedef ccGraph::vertex_descriptor VertexID;
	typedef ccGraph::edge_descriptor   EdgeID;


	inline static void increasingSwap(u64& left, u64& right)
	{
		if (left > right)
			swap(left, right);
	}

	inline static string concateInts(u64 left, u64 right, u64 domain)
	{
		auto scale = left * (pow(10,floor(log10(domain)) + 1)); //scale
		return (ToString(scale) + ToString(right)); //h1||h2
	}

	
	inline static string concateIntsIncr(u64 left, u64 right, u64 domain)
	{
		increasingSwap(left, right);
		return concateInts(left, right, domain);
	}

	inline static string Edge2StringIncr(EdgeID edge, u64 domain)
	{
		auto a = edge.m_source;
		auto b = edge.m_target;
	
		//if (a > b)
			increasingSwap(a, b);

		return concateInts(a, b, domain);
	}

	//TODO: optimize
	inline static  std::vector<bool> bitVector2BoolVector(BitVector& bit_vector) {
		std::vector<bool> res(bit_vector.size());
		for (u64 i = 0; i < bit_vector.size(); i++)
		{
			res[i] = bit_vector[i];
		}
		return res;
	}

	inline static void print_BoolVector(std::vector<bool> vec)
	{
		for (u64 i = 0; i < vec.size(); i++)
		{
			std::cout << vec[i] << "";
		}
		std::cout << "\n";
	}



	class MyVisitor : public boost::default_dfs_visitor
	{
	private:
		boost::shared_ptr< std::vector<EdgeID>> mDfs_visitor; // TODO: remove this
		boost::shared_ptr< std::vector<EdgeID>> mDfs_component;
		boost::shared_ptr<std::vector<std::vector<EdgeID>>> mDfs_circles;
		boost::shared_ptr<std::set<EdgeID>> mDfs_back_edges;

	public:
		MyVisitor() : mDfs_visitor(new std::vector<EdgeID>())
			, mDfs_component(new std::vector<EdgeID>())
			, mDfs_circles(new std::vector<std::vector<EdgeID>>())
			, mDfs_back_edges(new std::set<EdgeID>())
		{}

		
		ccGraph mCuckooGraph;
		
		int mInputSize, mNumHashs, mNumBins, mSigma;
		std::vector<u64> hashes1;
		std::vector<u64> hashes2;
		std::set<std::set<u64>> mBadEdges;
		std::map<std::set<u64>, int> mEdgeIdxMap_new; //TODO: edgeId mapping // std::map<EdgeID, int> mEdgeIdxMap2;



		void init(u64 inputSize, u64 numHashs, u64 numBins, u64 sigma);
		void buidingGraph(span<block> items);
#if 0
		/*void initialize_vertex(const ccGraph::vertex_descriptor& s, const ccGraph& g) const {
		std::cout << "initialize_vertex: " << s<< std::endl;
		}
		*/
		void start_vertex(const ccGraph::vertex_descriptor& s, const ccGraph& g) const {
			//std::cout << "start_vertex: " << s << std::endl;
		}

		void discover_vertex(const ccGraph::vertex_descriptor& s, const ccGraph& g) const {
		//	std::cout << "discover_vertex: " << s << std::endl;
		}
		void examine_vertex(const ccGraph::vertex_descriptor& s, const ccGraph& g) const {
			//std::cout << "examine_vertex: " << s << std::endl;
		}
		/*void examine_edge(const Graph::edge_descriptor &e, const Graph &g) const {
		std::cout << "Examine edge: " << e<< std::endl;
		}*/
#endif
		
		void tree_edge(const ccGraph::edge_descriptor& e, const ccGraph& g) const {

			if (mBadEdges.find({ e.m_source,e.m_target }) != mBadEdges.end())  //bad item
			{
				//std::cout << "back_edge back_edge: " << e << std::endl;
				mDfs_back_edges->insert(e);
				mDfs_component->push_back(e); //last is back_edge
				mDfs_circles->push_back(*mDfs_component); // add this circle to dfs_circles
				mDfs_component->clear(); //for next circle
			}
			else {
				mDfs_visitor->push_back(e);
				mDfs_component->push_back(e);
				//std::cout << "tree_edge: " << e << std::endl;
			}
		}
		

		void back_edge(const ccGraph::edge_descriptor& e, const ccGraph& g) const {

			// since the graph is undirected, if tree_edge is (0,1) then back_edge is (1,0)
			//we want to remove this back_edge by remembering dfs_tree_edges, and check the condition in void back_edge

#ifdef BAD_DFS
		  if(std::find(mDfs_visitor->begin(), mDfs_visitor->end(), e) == mDfs_visitor->end() 
			  && mDfs_back_edges->find(e) == mDfs_back_edges->end())
			{
				//std::cout << "back_edge back_edge: " << e << std::endl;
				mDfs_back_edges->insert(e);
				mDfs_component->push_back(e); //last is back_edge
				mDfs_circles->push_back(*mDfs_component); // add this circle to dfs_circles
				mDfs_component->clear(); //for next circle
			}
#endif // BAD_DFS

			//std::cout << "back_edge: " << e << std::endl;
		}
		void forward_or_cross_edge(const ccGraph::edge_descriptor& e, const ccGraph& g) const {
			//std::cout << "forward_or_cross_edge: " << e << std::endl;
			
			// since the graph is undirected, if tree_edge is (0,1) then back_edge is (1,0)
			// forward_or_cross_edge is "potential" back_edge

#ifdef BAD_DFS
			if (std::find(mDfs_visitor->begin(), mDfs_visitor->end(), e) == mDfs_visitor->end()
				&& mDfs_back_edges->find(e) == mDfs_back_edges->end())
#endif // BAD_DFS
			{
				//std::cout << "back_edge back_edge: " << e << std::endl;
				mDfs_back_edges->insert(e);
				mDfs_component->push_back(e); //last is back_edge
				mDfs_circles->push_back(*mDfs_component); // add this circle to dfs_circles
				mDfs_component->clear(); //for next circle
			}
		}


		//void finish_vertex(const ccGraph::vertex_descriptor& s, const ccGraph& g) const {
		//	//std::cout << "finish_vertex: " << s << std::endl;

		//	//if (s== start_Vertex)
		//	//{
		//	//	std::cout << "finish_vertex finish_vertex: " << s << std::endl;
		//	//	std::cout << "mDfs_component->size(): " << mDfs_component->size() << std::endl;

		//	//	if (mDfs_component->size() > 0) //no circle
		//	//	{
		//	//		mDfs_non_connected_components->push_back(*mDfs_component); // 
		//	//		mDfs_component->clear(); //for next circle
		//	//	//	isConnectedComponent = false;
		//	//	}

		//	//	if (isConnectedComponent)
		//	//	{
		//	//		//if(mDfs_component->size() > 0))
		//	//		mDfs_connected_components->push_back(*mDfs_circles); // 
		//	//		mDfs_component->clear(); //for next circle
		//	//		mDfs_circles->clear(); //for next circle
		//	//		isConnectedComponent = false;
		//	//	}
		//	//}
		//}

		
		std::vector<std::vector<EdgeID>>& GetDfsCircles() const { return *mDfs_circles; }
		std::set<EdgeID>& GetDfsBackEdge() const { return *mDfs_back_edges; }
		std::vector<EdgeID>& GetDfsVisitor() const { return *mDfs_visitor; }
		
	};
}
