#include "stdafx.h"
#include "Logger.h"
#include "GIArray.h"
#include "DBLayer.h"

void GIArray::Init()
{
	giArray_updateHWND = NULL;
	giArray_updateMSG = 0;
	giArray_count = 0;
	InitializeCriticalSection( &cs_gi_array );
	int i;
	for( i=0; i<GA_MAX_ITEMS; i++ ) gi_array[i] = new GroundItem();
}

void GIArray::Free()
{
	Lock();
	int i;
	for( i=0; i<GA_MAX_ITEMS; i++ )
	{
		if( gi_array[i] != NULL )
		{
			delete gi_array[i];
			gi_array[i] = NULL;
		}
	}
	giArray_count = 0;
	Unlock();
	DeleteCriticalSection( &cs_gi_array );
}

int GIArray::AddGIInfo( GroundItem *pItemInfo )
{
	Lock();
	// Add new npc info
	int idx = FindFreeIndex();
	if( idx == -1 )
	{
		Unlock();
		return -1;
	}
	if( !gi_array[idx] ) gi_array[idx] = new GroundItem();
	if( !gi_array[idx] )
	{
		Unlock();
		return -1;
	}
	memcpy( gi_array[idx], pItemInfo, sizeof(class GroundItem) );
	giArray_count++;
	Unlock();
	PostUpdateMessage();
	//log_error( LOG_OK, "GIArray: Added new[%d]: [%S] [%S] (%d,%d,%d) tpml %u, oid %u\n",
	//	idx,
	//	name, title, x,y,z, templateID, objectID );
	return idx;
}

void GIArray::UpdateGIInfo( int idx, GroundItem *pItemInfo )
{
	Lock();
	// update info on existing
	if( !gi_array[idx] )
	{
		log_error( LOG_ERROR, "GIArray_UpdateGIInfo(): idx[%d] is free!\n", idx );
		// try to allocate!
		gi_array[idx] = new GroundItem();
		if( !gi_array[idx] )
		{
			Unlock();
			return;
		}
	}
	if( gi_array[idx]->isUnused() ) log_error( LOG_ERROR, "GIArray_UpdateGIInfo(): idx[%d] is unused!\n", idx );
	memcpy( gi_array[idx], pItemInfo, sizeof(class GroundItem) );
	Unlock();
	PostUpdateMessage();
	//log_error( LOG_OK, "GIArray: Updated[%d]: [%S] [%S] (%d,%d,%d) tpml %u, oid %u\n",
	//	idx,
	//	name, title, x,y,z, templateID, objectID );
}

void GIArray::DelGIByArrayIdx( int idx )
{
	if( idx<0 || (idx>=GA_MAX_ITEMS) ) return;
	Lock();
	if( gi_array[idx] )
	{
		gi_array[idx]->setUnused();
		giArray_count--;
		//log_error( LOG_OK, "GIArray: deleted %u from index OK\n", objectID, idx );
	}
	Unlock();
	PostUpdateMessage();
}

void GIArray::DeleteAll()
{
	Lock();
	int i;
	for( i=0; i<GA_MAX_ITEMS; i++ )
	{
		if( gi_array[i] ) gi_array[i]->setUnused();
	}
	giArray_count = 0;
	Unlock();
	PostUpdateMessage();
}

void GIArray::Lock()
{
	EnterCriticalSection( &cs_gi_array );
}

void GIArray::Unlock()
{
	LeaveCriticalSection( &cs_gi_array );
}

int GIArray::FindGIByObjectID( unsigned int objectID )
{
	int ret = -1;
	int i;
	Lock();
	for( i=0; i<GA_MAX_ITEMS; i++ )
	{
		if( gi_array[i] == NULL ) continue;
		if( gi_array[i]->objectID == objectID )
		{
			ret = i;
			break;
		}
	}
	Unlock();
	return ret;
}

int GIArray::FindFreeIndex()
{
	int i;
	Lock();
	for( i=0; i<GA_MAX_ITEMS; i++ )
	{
		if( gi_array[i] == NULL )
		{
			gi_array[i] = new GroundItem(); // allocate new 
			gi_array[i]->setUnused(); // and set as unused
			Unlock();
			return i;
		}
		if( gi_array[i]->isUnused() )
		{
			Unlock();
			return i;
		}
	}
	Unlock();
	log_error( LOG_ERROR, "GIArray_FindFreeIndex(): canot find free index, limit is %d!\n", GA_MAX_ITEMS );
	return -1;
}

void GIArray::SetUpdateCommand( HWND hWnd, UINT uMsg )
{
	giArray_updateHWND = hWnd;
	giArray_updateMSG = uMsg;
}

void GIArray::PostUpdateMessage()
{
	if( !giArray_updateHWND ) return;
	if( giArray_updateMSG < WM_USER ) return;
	PostMessage( giArray_updateHWND, giArray_updateMSG, 0, 0 );
}

void GIArray::DisplayToConsole()
{
	int i = 0;
	unsigned int nDisplayed = 0;
	log_error( LOG_USERAI, "=== Ground items: %d ===\n", giArray_count );
	if( giArray_count > 0 )
	{
		Lock();
		for( i=0; i<GA_MAX_ITEMS; i++ )
		{
			if( gi_array[i]->isUnused() )
				log_error( LOG_USERAI, "GI: %2d: <unused>\n", i );
			else
			{
				wchar_t itemName[256] = {0};
				DBLayer_getItemName( gi_array[i]->itemID, itemName, 255 );
				log_error( LOG_USERAI, "GI: %2d: itemID %u, objectID %u, count %d [%s]\n", i,
					gi_array[i]->itemID, gi_array[i]->objectID,
					gi_array[i]->count, itemName );
				nDisplayed++;
			}
			if( nDisplayed >= giArray_count ) break;
		}
		Unlock();
	}
	log_error( LOG_USERAI, "=== Ground items End ===\n" );
}
