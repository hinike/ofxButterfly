#include <assert.h>
#include <iostream>
#include "mesh.hpp"
#include "error.hpp"

namespace gfx
{
    
    Vertex WingedEdge::AddVertex(GLfloat x, GLfloat y, GLfloat z)
    {
        Vertex v(x, y, z);
        vertexList[v]; /* ensure the key exists */
        return v;
    }
    
    Edge WingedEdge::AddEdge(const Edge& e)
    {
        return AddEdge(e.V1(), e.V2());
    }
    
    Edge WingedEdge::AddEdge(const Vertex& v1, const Vertex& v2)
    {
        Edge e(v1, v2);
        
        /* get them in the vertexList */
        
        vertexList[v1].insert(e);
        vertexList[v2].insert(e);
        
        /* setup edge list */
        edgeListMap[e].edges.insert(e);
        edgeListMap[e].vertices.insert(v1);
        edgeListMap[e].vertices.insert(v2);
        
        return e;
    }
    
    Face WingedEdge::AddFace(const Edge& e1, const Edge& e2, const Edge& e3)
    {
        Face f(e1, e2, e3);
        
        /* ensure below */
        AddEdge(e1.V1(), e1.V2());
        AddEdge(e2.V1(), e2.V2());
        AddEdge(e3.V1(), e3.V2());
        
        /* edit face lists */
        faceList[f].insert(e1);
        faceList[f].insert(e2);
        faceList[f].insert(e3);
        
        edgeListMap[e1].faces.insert(f);
        edgeListMap[e2].faces.insert(f);
        edgeListMap[e3].faces.insert(f);
        
        edgeListMap[e1].vertices.insert(e1.V1());
        edgeListMap[e1].vertices.insert(e1.V2());
        edgeListMap[e2].vertices.insert(e2.V1());
        edgeListMap[e2].vertices.insert(e2.V2());
        edgeListMap[e3].vertices.insert(e3.V1());
        edgeListMap[e3].vertices.insert(e3.V2());
        
        return f;
    }
    
    void WingedEdge::Draw()
    {
        for (auto it = edgeListMap.begin(); it != edgeListMap.end(); ++it)
            it->first.Draw();
    }
    
    // Interface function, performs butterfly subdivision that does not account for the internal special cases.
    // The subdivision only accounts for the boundaries and 6 regular vertices.
    WingedEdge WingedEdge::ButterflySubdivide()
    {
        return Subdivide(false);
    }
    
    // Interface function, performs linear subdivision on the mesh.
    WingedEdge WingedEdge::LinearSubdivide()
    {
        return Subdivide(true);
    }
    
    // Internal subdivision work function.
    WingedEdge WingedEdge::Subdivide(bool linear)
    {
        WingedEdge mesh;
        std::set<Edge> edges;
        
        for (auto face = faceList.begin(); face != faceList.end(); ++face)
        {
            
            /* massive assumption that there is 3 edges in our face */
            Edge e1 = face -> first.E1();
            Edge e2 = face -> first.E2();
            Edge e3 = face -> first.E3();

            bool success = true;
            /* might need to verify this doesn't pick duplicates */
            Vertex v1 = GetAdjacentVertex(face->first, e1, success);
            Vertex v2 = GetAdjacentVertex(face->first, e2, success);
            Vertex v3 = GetAdjacentVertex(face->first, e3, success);
            
            if(!success)
            {
                throw new RuntimeError("Something is wrong with the mesh topology");
            }
            
            /* guarantee we know what e2 is */
            if (v1 == e3.V1() || v1 == e3.V2())
            {
                Edge tmp = e3;
                e3 = e2;
                e2 = tmp;
            }
            
            Vertex v4 = SubdivideEdge(face->first, e1, v1, linear);
            Vertex v5 = SubdivideEdge(face->first, e2, v2, linear);
            Vertex v6 = SubdivideEdge(face->first, e3, v3, linear);
            
            performTriangulation(mesh, v1, v2, v3, v4, v5, v6);
            
        }
        
        /*
         std::cout << "Subdivide info: " << std::endl;
         std::cout << "VertexList: " << mesh.NumVertices() << std::endl;
         std::cout << "EdgeList: " << mesh.NumEdges() << std::endl;
         std::cout << "FaceList: " << mesh.NumFaces() << std::endl;
         */
        
        return mesh;
    }
    
    WingedEdge WingedEdge::BoundaryTrianglularSubdivide()
    {
        
        WingedEdge mesh;
        std::set<Edge> edges;
        
        for (auto face = faceList.begin(); face != faceList.end(); ++face)
        {
            
            /* massive assumption that there is 3 edges in our face */
            Edge e1 = face -> first.E1();
            Edge e2 = face -> first.E2();
            Edge e3 = face -> first.E3();
            
            // Compute the vertices opposite the cooresponding indiced edges.
            bool success = true;
            Vertex v1 = GetAdjacentVertex(face->first, e1, success);
            Vertex v2 = GetAdjacentVertex(face->first, e2, success);
            Vertex v3 = GetAdjacentVertex(face->first, e3, success);
            
            if(!success)
            {
                throw new RuntimeError("Error : Winged Edge topology is malformed!");
            }
            
            // Compute boundary predicates.
            bool b1, b2, b3;
            b1 = getNumAdjacentFaces(e1) == 1;
            b2 = getNumAdjacentFaces(e2) == 1;
            b3 = getNumAdjacentFaces(e3) == 1;
            
            // -- Count the number of edges that are on the boundary.
            int boundary_count = 0;
            boundary_count = b1 ? boundary_count + 1 : boundary_count;
            boundary_count = b2 ? boundary_count + 1 : boundary_count;
            boundary_count = b3 ? boundary_count + 1 : boundary_count;
            
            // Non Boundary --> do not subdivide the face.
            if(boundary_count == 0)
            {
                e1 = mesh.AddEdge(e1);
                e2 = mesh.AddEdge(e2);
                e2 = mesh.AddEdge(e2);
                
                mesh.AddFace(e1, e2, e3);
                continue;
            }

            // 1 boundary --> subdivide into 2 vertices.
            if(boundary_count == 1)
            {

                Vertex v_new, v_old1, v_old2, v_old3;
                
                if(b1)
                {
                    v_new = SubdivideEdge(face -> first, e1, v1, false);
                    v_old1 = v1;
                    v_old2 = v2;
                    v_old3 = v3;
                }
                else if(b2)
                {
                    v_new = SubdivideEdge(face -> first, e2, v2, false);
                    v_old1 = v2;
                    v_old2 = v3;
                    v_old3 = v1;
                }
                else
                {
                    v_new = SubdivideEdge(face -> first, e3, v3, false);
                    v_old1 = v3;
                    v_old2 = v1;
                    v_old3 = v2;
                }
                
                // face1
                e1 = mesh.AddEdge(v_new, v_old1);
                e2 = mesh.AddEdge(v_new, v_old2);
                e3 = mesh.AddEdge(v_old1, v_old2);
                mesh.AddFace(e1, e2, e3);
                
                // Face 2.
                e2 = mesh.AddEdge(v_new, v_old3);
                e3 = mesh.AddEdge(v_old1, v_old3);
                mesh.AddFace(e1, e2, e3);
                
                continue;
            }
            
            // 2 - 3 boundaries. Perform the full 4 face triangulation.
            
            Vertex vn1 = SubdivideEdge(face -> first, e1, v1, !b1);
            Vertex vn2 = SubdivideEdge(face -> first, e2, v2, !b2);
            Vertex vn3 = SubdivideEdge(face -> first, e3, v3, !b3);
            
            if(boundary_count == 3)
            {
                performTriangulation(mesh,
                                     v1, v2, v3,
                                     vn1, vn2, vn3);
                
                continue;
            }
            
            if(boundary_count > 3)
            {
                throw new RuntimeError("Face has more than 3 edges. This is not a triangle!");
            }
            
            // -- 2 boundary code.
            Vertex v_new1, v_new2, v_old1, v_old2, v_old3;
            
            if(!b1)
            {
                v_old1 = v1;
                v_old2 = v2;
                v_old3 = v3;
                v_new1 = vn2;
                v_new2 = vn3;
            }
            else if(!b2)
            {
                v_old1 = v2;
                v_old2 = v3;
                v_old3 = v1;
                v_new1 = vn3;
                v_new2 = vn1;
            }
            else
            {
                v_old1 = v3;
                v_old2 = v1;
                v_old3 = v2;
                v_new1 = vn1;
                v_new2 = vn2;
            }
            
            /*
             *
             */
            
            // FIXME : add the proper faces here.
            
            // Boundary triangles.
            e1 = mesh.AddEdge(v_old1, v_new1);
            e2 = mesh.AddEdge(v_old1, v_new2);
            e3 = mesh.AddEdge(v_new1, v_new2);
            mesh.AddFace(e1, e2, e3);

            
            e1 = mesh.AddEdge(v_old3, v_new1);
            e2 = mesh.AddEdge(v_old3, v_new2);
            e3 = mesh.AddEdge(v_new1, v_new2);
            mesh.AddFace(e1, e2, e3);
            
            // Constant edge triangle.
            e1 = mesh.AddEdge(v_old2, v_new2);
            e2 = mesh.AddEdge(v_old3, v_new2);
            e3 = mesh.AddEdge(v_old3, v_old2);
            mesh.AddFace(e1, e2, e3);
            
        }
        
        return mesh;
    }
    
    // -- A whimsical subdivision that creates pascal's triangle like structures.
    WingedEdge WingedEdge::SillyPascalSubdivide()
    {
        WingedEdge mesh;
        std::set<Edge> edges;
        
        for (auto face = faceList.begin(); face != faceList.end(); ++face)
        {
            
            /* massive assumption that there is 3 edges in our face */
            Edge e1 = face -> first.E1();
            Edge e2 = face -> first.E2();
            Edge e3 = face -> first.E3();
            
            /* might need to verify this doesn't pick duplicates */
            Vertex v1 = e1.V1();
            Vertex v2 = e1.V2();
            Vertex v3 = (e2.V1() == v1 || e2.V1() == v2) ? e2.V2() : e2.V1();
            
            /* guarantee we know what e2 is */
            if (v1 == e3.V1() || v1 == e3.V2())
            {
                Edge tmp = e3;
                e3 = e2;
                e2 = tmp;
            }
            
            int f1, f2, f3;
            f1 = getNumAdjacentFaces(e1);
            f2 = getNumAdjacentFaces(e2);
            f3 = getNumAdjacentFaces(e3);
            
            // Do not subdivide and do not incorporate non boundary faces.
            // This is the part the creates the pascal behavior,
            if(f1 == 2 && f2 == 2 && f3 == 2)
            {
                continue;
            }
            
            bool success = true;
            Vertex v4 = SubdivideEdge(face->first, e1, GetAdjacentVertex(face->first, e1, success), false);
            Vertex v5 = SubdivideEdge(face->first, e2, GetAdjacentVertex(face->first, e2, success), false);
            Vertex v6 = SubdivideEdge(face->first, e3, GetAdjacentVertex(face->first, e3, success), false);
            
            // A half hearted success check.
            if(!success)
            {
                throw RuntimeError("WindgedEdge Error: Something is wrong with the mesh to be subdivided.");
            }
            
            {
                e1 = mesh.AddEdge(v1, v4);
                e2 = mesh.AddEdge(v1, v5);
                e3 = mesh.AddEdge(v5, v4);
                mesh.AddFace(e1, e2, e3);
            }
            
            {
                e1 = mesh.AddEdge(v4, v2);
                e2 = mesh.AddEdge(v4, v6);
                e3 = mesh.AddEdge(v6, v2);
                mesh.AddFace(e1, e2, e3);
            }
            
            {
                e1 = mesh.AddEdge(v5, v6);
                e2 = mesh.AddEdge(v5, v3);
                e3 = mesh.AddEdge(v3, v6);
                mesh.AddFace(e1, e2, e3);
            }
            
            {
                e1 = mesh.AddEdge(v6, v5);
                e2 = mesh.AddEdge(v6, v4);
                e3 = mesh.AddEdge(v4, v5);
                mesh.AddFace(e1, e2, e3);
            }
            
        }
        
        /*
         std::cout << "Subdivide info: " << std::endl;
         std::cout << "VertexList: " << mesh.NumVertices() << std::endl;
         std::cout << "EdgeList: " << mesh.NumEdges() << std::endl;
         std::cout << "FaceList: " << mesh.NumFaces() << std::endl;
         */
        
        return mesh;
    }
    
    
    // Adds 4 sub triangles based on three original vertices and 3 new vertices to the given mesh.
    // FIXME : Add better documentation and understanding to this function.
    void WingedEdge::performTriangulation(WingedEdge &mesh,
                                          Vertex &v1, Vertex &v2, Vertex &v3,
                                          Vertex &v4, Vertex &v5, Vertex &v6)
    {
        bool success = true;
        
        Edge e1, e2, e3;
        
        // A half hearted success check.
        if(!success)
        {
            throw RuntimeError("WingedEdge Error: Something is wrong with the mesh to be subdivided.");
        }
        
        // Face 1.
        e1 = mesh.AddEdge(v1, v5);
        e2 = mesh.AddEdge(v1, v6);
        e3 = mesh.AddEdge(v5, v6);
        mesh.AddFace(e1, e2, e3);
        
        
        // Face 2.
        e1 = mesh.AddEdge(v2, v4);
        e2 = mesh.AddEdge(v2, v6);
        e3 = mesh.AddEdge(v4, v6);
        mesh.AddFace(e1, e2, e3);
        //
        
        // Face 3.
        e1 = mesh.AddEdge(v3, v4);
        e2 = mesh.AddEdge(v3, v5);
        e3 = mesh.AddEdge(v4, v5);
        mesh.AddFace(e1, e2, e3);
        
        
        // Face 4.
        e1 = mesh.AddEdge(v4, v5);
        e2 = mesh.AddEdge(v4, v6);
        e3 = mesh.AddEdge(v5, v6);
        mesh.AddFace(e1, e2, e3);
        
    }
    
    /* This functions computes the new butterfly vertices based on the points in the stencil of the given edge.
     *FIXME : http://mrl.nyu.edu/~dzorin/papers/zorin1996ism.pdf Page 3.
     * The special internal cases still need to be implemented.
     *
     * Only the degree 6 vertice cases and boundary cases have been implemented for butterfly.
     *
     * FIXME : add a type variable to determine what type of interpolation should be used for the edges.
     *         Currently it always uses the butterfly scheme.
     *
     * REQUIRES : e is in f1. b1 is in f1. b1 is not in e.
     *
     */
    Vertex WingedEdge::SubdivideEdge(const Face& f1, Edge& e, Vertex b1, bool linear)
    {
        /* get our a midpoint */
        Vertex v;
        v = e.V1() / 2.0;
        v = v + (e.V2() / 2.0);
        
        if(linear)
        {
            return v;
        }
        
        Vertex v_original = v;
        v_original = v;
        
        // Flag for whether we are in theboundary case or not.
        bool boundary = false;
        
        do
        {
            
            bool success = true;
            
            Face f2 = GetAdjacentFace(f1, e, success);
            
            if(!success)
            {
                boundary = true;
                break;
            }
            
            /* get our opposing face's b point */
            Vertex b2 = GetAdjacentVertex(f2, e, success);
            
            if(!success)
            {
                boundary = true;
                break;
            }
            
            v = v + (b1/8.0);
            v = v + (b2/8.0);
            
            /* time to get our c points */
            std::set<Edge> edges;
            edges.insert(f1.E1());
            edges.insert(f1.E2());
            edges.insert(f1.E3());
            for (auto edge = edges.begin(); edge != edges.end(); ++edge)
            {
                if (*edge != e)
                {
                    v = v - (GetAdjacentFaceVertex(f1, *edge, success)/16.0);
                    
                    if(!success)
                    {
                        boundary = true;
                        break;
                    }
                }
            }
            
            edges.erase(edges.begin(), edges.end());
            edges.insert(f2.E1());
            edges.insert(f2.E2());
            edges.insert(f2.E3());
            for (auto edge = edges.begin(); edge != edges.end(); ++edge)
            {
                if (*edge != e)
                {
                    v = v - (GetAdjacentFaceVertex(f2, *edge, success)/16.0);
                    if(!success)
                    {
                        boundary = true;
                        break;
                    }
                }
            }
            
        }
        while(false);
        
        
        if(boundary)
        {
            /*
             * Proceed with boundary case.
             */
            
            // Extract the 4 vertices.
            Vertex v1, v2, v3, v4;
            v1 = e.V1();
            v2 = e.V2();
            v3 = getOtherBoundaryVertice(v1, e);
            v4 = getOtherBoundaryVertice(v2, e);
            
            return v1*9/16 + v2*9/16 - v3/16 - v4/16;
        }
        
        return v;
    }
    
    
    // --  Windged Edge Mesh topology navigation and transversal helper functions.
    
    
    Vertex WingedEdge::GetAdjacentFaceVertex(const Face& face, const Edge& edge, bool &success)
    {
        success = true;
        Face f2 = GetAdjacentFace(face, edge, success);
        
        // Bogus return with false success flag.
        if(!success)
        {
            return edge.V1();
        }
        
        return GetAdjacentVertex(f2, edge, success);
    }
    
    // Returns. the vertice on the face that is not in the given edge.
    // FIXME : Do we really need to check successes? This should be guranteed.
    Vertex WingedEdge::GetAdjacentVertex(const Face& face, const Edge& edge, bool &success)
    {
        success = true;
        
        if (face.E1() != edge)
            return (face.E1().V1() == edge.V1()) ? face.E1().V2() : face.E1().V1();
        
        else if (face.E2() != edge)
            return (face.E2().V1() == edge.V1()) ? face.E2().V2() : face.E2().V1();
        
        else if (face.E3() != edge)
            return (face.E3().V1() == edge.V1()) ? face.E3().V2() : face.E3().V1();
        
        // Bugus return.
        success = false;
        return edge.V1();
        //throw RuntimeError("Couldn't find Adjacent Vertex");
    }
    
    Face WingedEdge::GetAdjacentFace(const Face& face, const Edge& edge, bool &success)
    {
        success = true;
        
        EdgeList edgeList = edgeListMap[edge];
        std::set<Face>::const_iterator it;
        for (it = edgeList.faces.begin(); it != edgeList.faces.end(); ++it)
        {
            if (*it != face)
                return *it;
        }
        
        // Bogus return on non success.
        success = false;
        return face;
        // throw RuntimeError("Couldn't find adjacent face.");
    }
    
    int WingedEdge::getNumAdjacentFaces(const Edge& edge)
    {
        EdgeList edgeList = edgeListMap[edge];
        
        return edgeList.faces.size();
    }
    
    Vertex WingedEdge::getOtherVertex(Edge &edge, Vertex &v)
    {
        Vertex v1 = edge.V1();
        return v1 == v ? edge.V2() : v1;
        
    }
    
    // Returns the boundary edge that is not e.
    Vertex WingedEdge::getOtherBoundaryVertice(Vertex &a, Edge &forbidden_edge)
    {
        std::set<Edge> edges = vertexList[a];
        for(Edge e : edges)
        {
            if(e != forbidden_edge && getNumAdjacentFaces(e) == 1)
            {
                return getOtherVertex(e, a);
            }
        }
        
        //throw RuntimeError("No other boundary edge was found. Something might be wrong with your mesh.");
        return a;
    }
    
    /* end */
}
