//
// Copyright (c) 2017-2019, Manticore Software LTD (http://manticoresearch.com)
// All rights reserved
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License. You should have
// received a copy of the GPL license along with this program; if you
// did not, you can find it at http://www.gnu.org/
//

/// @file _accumulator.h_
/// Declarations for the stuff related to accumulator


#ifndef _accumulator_
#define _accumulator_

#include "sphinx.h"
#include "sphinxint.h"

struct StoredQueryDesc_t
{
	CSphVector<CSphFilterSettings>	m_dFilters;
	CSphVector<FilterTreeItem_t>	m_dFilterTree;

	CSphString						m_sQuery;
	CSphString						m_sTags;
	uint64_t						m_uQUID = 0;
	bool							m_bQL = true;
};

class StoredQuery_i : public ISphNoncopyable, public StoredQueryDesc_t
{
public:
	virtual ~StoredQuery_i() {}
};

// commands that got replicated, transactions
enum class ReplicationCommand_e
{
	PQUERY_ADD = 0,
	PQUERY_DELETE,
	TRUNCATE,
	CLUSTER_ALTER_ADD,
	CLUSTER_ALTER_DROP,

	TOTAL
};

// command trait
struct ReplicationCommand_t
{
	// common
	ReplicationCommand_e	m_eCommand { ReplicationCommand_e::TOTAL };
	CSphString				m_sIndex; // move to accumulator
	CSphString				m_sCluster;

	// add
	CSphScopedPtr<StoredQuery_i> m_pStored { nullptr };

	// delete
	CSphVector<uint64_t>	m_dDeleteQueries;
	CSphString				m_sDeleteTags;

	// truncate
	bool					m_bReconfigure = false;
	CSphScopedPtr<CSphReconfigureSettings> m_tReconfigure { nullptr };

	bool					m_bCheckIndex = true;
	bool					m_bIsolated = false;
};

class RtIndex_i;

/// indexing accumulator
class RtAccum_t
{
public:
	DWORD							m_uAccumDocs {0};
	CSphTightVector<CSphWordHit>	m_dAccum;
	CSphTightVector<CSphRowitem>	m_dAccumRows;
	CSphVector<DocID_t>			m_dAccumKlist;
	CSphTightVector<BYTE>		m_dBlobs;
	CSphVector<DWORD>			m_dPerDocHitsCount;
	CSphVector<ReplicationCommand_t *> m_dCmd;

	bool						m_bKeywordDict {true};
	CSphDictRefPtr_c			m_pDict;
	CSphDict *					m_pRefDict = nullptr; // not owned, used only for ==-matching

private:
	CSphRefcountedPtr<ISphRtDictWraper>	m_pDictRt;
	bool						m_bReplace = false;		///< insert or replace mode (affects CleanupDuplicates() behavior)
	BlobRowBuilder_i *			m_pBlobWriter {nullptr};
	RowID_t						m_tNextRowID {0};
	void						ResetDict ();

	// FIXME!!! index is unlocked between add data and commit or at begin and end
	RtIndex_i *					m_pIndex = nullptr;		///< my current owner in this thread

public:
					explicit RtAccum_t ( bool bKeywordDict );
					~RtAccum_t();

	void			SetupDict ( const RtIndex_i * pIndex, CSphDict * pDict, bool bKeywordDict );
	void			Sort ();

	void CleanupPart();
	void Cleanup();

	void			AddDocument ( ISphHits * pHits, const CSphMatch & tDoc, bool bReplace, int iRowSize,
		const char ** ppStr, const VecTraits_T<int64_t> & dMvas );
	RtSegment_t *	CreateSegment ( int iRowSize, int iWordsCheckpoint );
	void			CleanupDuplicates ( int iRowSize );
	void			GrabLastWarning ( CSphString & sWarning );
	void			SetIndex ( RtIndex_i * pIndex );

	RowID_t			GenerateRowID();
	void			ResetRowID();

	RtIndex_i * GetIndex() const { return m_pIndex; }
	ReplicationCommand_t * AddCommand();
};

#endif // _accumulator_