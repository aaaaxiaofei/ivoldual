/// \file ijkdualtable.h
/// Class containing dual lookup table of isosurface vertices.
/// Version 0.2.0

/*
  IJK: Isosurface Jeneration Kode
  Copyright (C) 2012-2016 Rephael Wenger

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _IJKDUALTABLE_
#define _IJKDUALTABLE_

#include "ijk.txx"
#include "ijkcube.txx"

/// Classes and routines for storing and manipulating 
///   dual isosurface lookup table.
namespace IJKDUALTABLE {

  // **************************************************
  // TYPES
  // **************************************************

  typedef int TABLE_INDEX;                    ///< Index of table entry.
  typedef unsigned char ISODUAL_VERTEX_INDEX; ///< Index of isosurface vertex.

  // Forward definition.
  class FIND_COMPONENT;
  
  // **************************************************
  // COMPUTE FUNCTIONS
  // **************************************************

  /// Compute complement index.
  template <typename NTYPE>
  inline TABLE_INDEX compute_complement
  (const TABLE_INDEX ival, const NTYPE num_table_entries)
  { return(num_table_entries-1-ival); }
    
  // **************************************************
  // ISODUAL TABLE
  // **************************************************

  /// Dual isosurface lookup table.
  /// Stores isosurface vertices and incident faces for each configuration 
  ///   of +/- labels at cube vertices.
  class ISODUAL_TABLE {

  protected:

    /// Entry in the dual isosurface lookup table.
    class ISODUAL_TABLE_ENTRY {

    public:
      int num_vertices;           ///< Number of dualiso vertices in cube.
      ISODUAL_TABLE_ENTRY();      ///< constructor
      ~ISODUAL_TABLE_ENTRY();     ///< destructor

      /// incident_isovertex[kf] = Isosurface vertex incident on face kf.
      ///       Face kf is dual to polytope edge kf.
      ISODUAL_VERTEX_INDEX * incident_isovertex;

      /// is_bipolar[ke] = True if polytope edge ke is bipolar.
      ///       Cube edge ke is dual to isosurface face kf.
      bool * is_bipolar;

      /// Allocate incident_isovert[] and is_bipolar[].
      void Allocate(const int num_poly_edges);

      bool Check(IJK::ERROR & error_msg) const;
      void FreeAll();                            // free all memory
    };


  public:

  /// Index of entry in isosurface lookup table.
  /// Define within ISODUAL_TABLE for use in templates.
  typedef IJKDUALTABLE::TABLE_INDEX TABLE_INDEX;    


  protected:

    int dimension;                  ///< Dimension
    int num_poly_vertices;          ///< Number of polytope vertices.
    int num_poly_edges;             ///< Number of polytope edges;
    ISODUAL_TABLE_ENTRY * entry;    ///< Array of dual isosurface table entries.
    long num_table_entries;         ///< Number of entries in table.

    /// Maximum number of vertices allowed for cube.
    int max_num_vertices; 

    /// True, if array entry[] is allocated.
    bool is_table_allocated;  

    /// Initialization routine.
    void Init(const int dimension);


  public:
    ISODUAL_TABLE();
    ISODUAL_TABLE(const int d);
    ~ISODUAL_TABLE();                ///< Destructor

    // Get functions.
    int Dimension() const            ///< Return dimension.
    { return(dimension); };
    int NumPolyVertices() const      ///< Return number of polytope vertices.
    { return(num_poly_vertices); };
    int NumPolyEdges() const         ///< Return number of polytope edges.
    { return(num_poly_edges); };

    /// Return number of lookup table entries.
    int NumTableEntries() const { return(num_table_entries); };

    /// Return complement of table index it
    int Complement(const int it) const
    { return(compute_complement(it, num_table_entries)); }

    /// Return number of vertices in isosurface patch for table entry \a it.
    int NumIsoVertices(const TABLE_INDEX it) const
    { return(entry[it].num_vertices); }; 

    /// Return index of isosurface vertex incident on face kf.
    /// Undefined if polytope edge k is not bipolar.
    /// @param it Index of table entry.
    /// @param kf Isosurface face kf, dual to polytope edge kf.
    ISODUAL_VERTEX_INDEX IncidentIsoVertex
    (const TABLE_INDEX it, const int kf) const
    { return(entry[it].incident_isovertex[kf]); };

    /// Return true if edge ke is bipolar.
    /// @param it Index of table entry.
    /// @param ke Polytope edge ke, dual to isosurface face ke.
    bool IsBipolar(const TABLE_INDEX it, const int ke) const
    { return(entry[it].is_bipolar[ke]); };

    /// Return true if vertex iv is positive.
    /// @param iv Vertex index.
    bool IsPositive(const TABLE_INDEX it, const int iv) const;

    /// Return maximum number of polytope vertices permitted in any table.
    /// Note: Even tables for polytopes of this size are probably impossible 
    ///   to compute/store.
    int MaxNumVertices() const { return(max_num_vertices); };

    /// Return true if table memory is allocated.
    bool IsTableAllocated() const
    { return(is_table_allocated); };

    // Set functions.
    void SetDimension(const int d);
    void SetNumPolyVertices(const int num_vertices);
    void SetNumPolyEdges(const int num_edges);

    /// Allocate table
    void SetNumTableEntries(const int num_table_entries);

    // Check functions
    bool CheckDimension(const int d) const;
    bool CheckDimension() const
    { return(CheckDimension(Dimension())); };
    bool CheckTable(IJK::ERROR & error_msg) const;
    bool Check(IJK::ERROR & error_msg) const;

    virtual void FreeAll();                     /// Free all memory.
  };

  typedef ISODUAL_TABLE * ISODUAL_TABLE_PTR;


  // **************************************************
  // ISODUAL CUBE TABLE
  // **************************************************

  /// ISODUAL_TABLE based on cube.
  class ISODUAL_CUBE_TABLE:public ISODUAL_TABLE {

  protected:
    bool flag_separate_neg;                 ///< If true, separate negative vertices.

    /// If true, always separate two diagonally opposite
    ///   positive or negative vertices.
    bool flag_always_separate_opposite;     

    /// Create table entries.
    /// @param flag_opposite_vertices If true, always separate two
    ///        diagonally opposite positive or negative vertices.
    void CreateTableEntries
      (const bool flag_separate_neg, const bool flag_separate_opposite);

  public:
    ISODUAL_CUBE_TABLE() {};
    ISODUAL_CUBE_TABLE(const int dimension);
    ISODUAL_CUBE_TABLE
      (const int dimension, const bool flag_opposite_vertices);
    ISODUAL_CUBE_TABLE
      (const int dimension, const bool separate_neg,
       const bool flag_separate_opposite);

    /// Return true if separate negative vertices.
    bool FlagSeparateNeg() const
    { return(flag_separate_neg); }

    // Set functions.
    void SetDimension(const int d);

    void Create(const int dimension);
    void Create(const int dimension, const bool flag_separate_opposite);
    void Create(const int dimension, const bool flag_separate_neg, 
                const bool flag_separate_opposite);

    /// Undefine function.
    void SetNumTableEntries(const int num_table_entries);
  };


  // **************************************************
  // ISODUAL CUBE TABLE AMBIG
  // **************************************************

  /// Isodual cube table plus ambiguity information.
  class ISODUAL_CUBE_TABLE_AMBIG:public ISODUAL_CUBE_TABLE {

  public:
    typedef unsigned char FACET_INDEX;          ///< Index of facet.
    typedef int FACET;          ///< Bits representing vertices in facet.
    typedef int FACET_SET;      ///< Bits representing set of facets.

  protected:
    bool * is_ambiguous;            ///< True for ambiguous configurations.
    FACET_SET * ambiguous_facet;    ///< k'th bit is 1 if facet k is ambiguous

    /// Number of ambiguous facets.
    FACET_INDEX * num_ambiguous_facets;           

    /// Number of active facets (with both positive and negative vertices.)
    FACET_INDEX * num_active_facets;

    void Init();                    ///< Initialization routine.
    void Alloc();                   ///< Allocate memory.
    void FreeAll();                 ///< Free all memory.

    /// Compute ambiguity information.
    void ComputeAmbiguityInformation();

    /// Compute number of active facets.
    void ComputeNumActiveFacets();

  public:
    
    // constructors
    ISODUAL_CUBE_TABLE_AMBIG();
    ISODUAL_CUBE_TABLE_AMBIG(const int dimension);
    ISODUAL_CUBE_TABLE_AMBIG
      (const int dimension, const bool separate_neg,
       const bool flag_separate_opposite);

    ~ISODUAL_CUBE_TABLE_AMBIG();                // destructor

    // get functions
    bool IsAmbiguous(const TABLE_INDEX it) const
    { return(is_ambiguous[it]); };
    bool IsFacetAmbiguous
      (const TABLE_INDEX it, const FACET_INDEX jf) const
    { return((ambiguous_facet[it] & ((1L) << jf)) != 0); };
    FACET_SET AmbiguousFacetBits(const TABLE_INDEX it) const
    { return(ambiguous_facet[it]); };
    FACET_INDEX NumAmbiguousFacets(const TABLE_INDEX it) const
    { return(num_ambiguous_facets[it]); };
    FACET_INDEX NumActiveFacets(const TABLE_INDEX it) const
    { return(num_active_facets[it]); };

    void Create(const int dimension);
    void Create(const int dimension, const bool flag_separate_opposite);
    void Create(const int dimension, const bool flag_separate_neg, 
                const bool flag_separate_opposite);
  };


  // **************************************************
  // CLASS FIND_COMPONENT
  // **************************************************

  /// Find connected component among cube vertices.
  class FIND_COMPONENT {

  protected:
    int dimension;
    int num_cube_vertices;
    bool * vertex_flag;
    int * component;

  public:
    FIND_COMPONENT(const int dimension);
    ~FIND_COMPONENT();

    // set functions
    void SetVertexFlags(const TABLE_INDEX ival);
    void NegateVertexFlags();
    void ClearAll();

    // get functions
    int Dimension() const
    { return(dimension); }
    bool VertexFlag(const int i) const
    { return(vertex_flag[i]); }
    int Component(const int i) const
    { return(component[i]); }
    int NumCubeVertices() const
    { return(num_cube_vertices); }

    /// Search starting at vertex i.
    /// @pre icomp is not zero.
    void Search(const int i, const int icomp);

    /// Search facet starting at vertex i.
    /// @pre Facet kf contains vertex i.
    /// @pre icomp is not zero.
    void SearchFacet(const int kf, const int i, const int icomp);

    /// Compute number of components.
    /// @param flag_positive If true, compute components of positive vertices.
    ///                      If false, compute components of negative vertices.
    int ComputeNumComponents
    (const int ientry, const bool flag_positive);

    /// Compute number of components in facet.
    /// @param flag_positive If true, compute components of positive vertices.
    ///                      If false, compute components of negative vertices.
    int ComputeNumComponentsInFacet
    (const int ientry, const int kf, const bool flag_positive);
  };


  // **************************************************
  // CLASS ISODUAL_CUBE_FACE_INFO
  // *************************************************

  /// Class with routines to query number of positive or negative vertices
  ///   in a cube facet and number of active facets.
  template <typename DTYPE, typename NTYPE, typename VTYPE>
  class ISODUAL_CUBE_FACE_INFO:public IJK::CUBE_FACE_INFO<DTYPE,NTYPE,VTYPE> {

  public:
    ISODUAL_CUBE_FACE_INFO() {};
    ISODUAL_CUBE_FACE_INFO(const DTYPE dimension):
      IJK::CUBE_FACE_INFO<DTYPE,NTYPE,VTYPE>(dimension) {};

    void ComputeNumCubeFacetBits
    (const TABLE_INDEX ientry, const NTYPE ifacet,
     NTYPE & num_zeros, NTYPE & num_ones) const;

    bool IsCubeFacetActive(const TABLE_INDEX ientry, const NTYPE ifacet) const;

    NTYPE ComputeNumActiveCubeFacets(const TABLE_INDEX ientry) const;
  };


  // **************************************************
  // CLASS ISODUAL_CUBE_FACE_INFO MEMBER FUNCTIONS
  // **************************************************


  template <typename DTYPE, typename NTYPE, typename VTYPE>
  void ISODUAL_CUBE_FACE_INFO<DTYPE,NTYPE,VTYPE>::
  ComputeNumCubeFacetBits(const TABLE_INDEX ientry, const NTYPE ifacet,
                          NTYPE & num_zeros, NTYPE & num_ones) const
  {
    num_ones = 0;
    num_zeros = 0;

    for (NTYPE j = 0; j < this->NumFacetVertices(); j++) {
      const NTYPE jv = this->FacetVertex(ifacet, j);
      const TABLE_INDEX mask = (TABLE_INDEX(1) << jv);
      if ((mask & ientry) == 0) {
        num_zeros++;
      }
      else {
        num_ones++;
      }
    }
  }


  template <typename DTYPE, typename NTYPE, typename VTYPE>
  bool ISODUAL_CUBE_FACE_INFO<DTYPE,NTYPE,VTYPE>::
  IsCubeFacetActive(const TABLE_INDEX ientry, const NTYPE ifacet) const
  {
    NTYPE num_zeros, num_ones;

    ComputeNumCubeFacetBits(ientry, ifacet, num_zeros, num_ones);
    if (num_zeros > 0 && num_ones > 0) 
      { return(true); }
    else 
      { return(false); }
  }

  template <typename DTYPE, typename NTYPE, typename VTYPE>
  NTYPE ISODUAL_CUBE_FACE_INFO<DTYPE,NTYPE,VTYPE>::
  ComputeNumActiveCubeFacets(const TABLE_INDEX ientry) const
  {
    NTYPE num_active_facets = 0;
    for (NTYPE ifacet = 0; ifacet < this->NumFacets(); ifacet++) {
      if (IsCubeFacetActive(ientry, ifacet)) 
        { num_active_facets++; }
    }

    return(num_active_facets);
  }


  // **************************************************
  // AMBIGUITY ROUTINES
  // **************************************************

  bool is_cube_ambiguous
  (const TABLE_INDEX ientry, FIND_COMPONENT & find_component);

  bool is_cube_facet_ambiguous
  (const TABLE_INDEX ientry, const ISODUAL_CUBE_TABLE_AMBIG::FACET_INDEX & kf, 
   FIND_COMPONENT & find_component);

  void compute_ambiguous_cube_facets
  (const TABLE_INDEX ientry, 
   const ISODUAL_CUBE_TABLE_AMBIG::FACET_INDEX num_facets,
   ISODUAL_CUBE_TABLE_AMBIG::FACET_SET & facet_set,  
   ISODUAL_CUBE_TABLE_AMBIG::FACET_INDEX & num_ambiguous_facets,
   FIND_COMPONENT & find_component);


  // **************************************************
  // UTILITY FUNCTIONS
  // **************************************************

  /// Calculate number of entries required in ISOSURFACE_TABLE.
  unsigned long calculate_num_entries(const int num_vert, const int num_colors);

  /// Convert integer to boolean flags.
  void convert2bool
    (const TABLE_INDEX ival, bool * flag, const unsigned int num_flags);
}

#endif
