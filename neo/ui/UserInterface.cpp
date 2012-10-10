/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).  

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "ListGUILocal.h"
#include "DeviceContext.h"
#include "Window.h"
#include "UserInterfaceLocal.h"

extern idCVar r_skipGuiShaders;		// 1 = don't render any gui elements on surfaces

idUserInterfaceManagerLocal	uiManagerLocal;
idUserInterfaceManager *	uiManager = &uiManagerLocal;

/*
===============================================================================

	idUserInterfaceManagerLocal

===============================================================================
*/

void idUserInterfaceManagerLocal::Init() {
	screenRect = idRectangle(0, 0, 640, 480);
	dc.Init();

	// TODO:  eviljoel:  Determine if the GUI patch will also work for the Demo and RoE
	// TODO:  eviljoel:  Make this look neater by using an array of CStrings
	idStrList* os2PrimaryPatchPaths = new idStrList( 1 );
	idStr* os2PrimaryPatch = new idStr( "guis/mainmenu-os2primary.guipatch" );
	os2PrimaryPatchPaths->Append( *os2PrimaryPatch );
	guiToPatchFilesMap.Set( "guis/mainmenu.gui", os2PrimaryPatchPaths );
}

void idUserInterfaceManagerLocal::Shutdown() {
	guiToPatchFilesMap.DeleteContents();
	guis.DeleteContents( true );
	demoGuis.DeleteContents( true );
	dc.Shutdown();
}

// Used with precaching (which is used with timedemos)
void idUserInterfaceManagerLocal::Touch( const char *name ) {
	idUserInterface *gui = Alloc();
	gui->InitFromFile( name );
//	delete gui;
}

// Note that resources are not quoted.  TODO:  eviljoel:  Should they be?
void idUserInterfaceManagerLocal::WritePrecacheCommands( idFile *f ) {

	const int c = guis.Num();

	for( int i = 0; i < c; i++ ) {
		common->Printf( "touchGui %s\n", guis[i]->Name() );
		f->Printf( "touchGui %s\n", guis[i]->Name() );
	}
}

void idUserInterfaceManagerLocal::SetSize( float width, float height ) {
	dc.SetSize( width, height );
}

void idUserInterfaceManagerLocal::BeginLevelLoad() {
	int c = guis.Num();
	for ( int i = 0; i < c; i++ ) {
		if ( (guis[ i ]->GetDesktop()->GetFlags() & WIN_MENUGUI) == 0 ) {
			guis[ i ]->ClearRefs();
			/*
			delete guis[ i ];
			guis.RemoveIndex( i );
			i--; c--;
			*/
		}
	}
}

void idUserInterfaceManagerLocal::EndLevelLoad() {
	int c = guis.Num();
	for ( int i = 0; i < c; i++ ) {
		if ( guis[i]->GetRefs() == 0 ) {
			//common->Printf( "purging %s.\n", guis[i]->GetSourceFile() );

			// use this to make sure no materials still reference this gui
			bool remove = true;
			for ( int j = 0; j < declManager->GetNumDecls( DECL_MATERIAL ); j++ ) {
				const idMaterial *material = static_cast<const idMaterial *>(declManager->DeclByIndex( DECL_MATERIAL, j, false ));
				if ( material->GlobalGui() == guis[i] ) {
					remove = false;
					break;
				}
			}
			if ( remove ) {
				delete guis[ i ];
				guis.RemoveIndex( i );
				i--; c--;
			}
		}
	}
}

void idUserInterfaceManagerLocal::Reload( bool all ) {
	ID_TIME_T ts;

	int c = guis.Num();
	for ( int i = 0; i < c; i++ ) {
		if ( !all ) {
			// Check the file timestamp and reload the ones that have been updated
			fileSystem->ReadFile( guis[i]->GetSourceFile(), NULL, &ts );
			if ( ts <= guis[i]->GetTimeStamp() ) {
				continue;
			}
		}

		guis[i]->InitFromFile( guis[i]->GetSourceFile() );
		common->Printf( "reloading %s.\n", guis[i]->GetSourceFile() );
	}
}

void idUserInterfaceManagerLocal::ListGuis() const {
	int c = guis.Num();
	common->Printf( "\n   size   refs   name\n" );
	size_t total = 0;
	int copies = 0;
	int unique = 0;
	for ( int i = 0; i < c; i++ ) {
		idUserInterfaceLocal *gui = guis[i];
		size_t sz = gui->Size();
		bool isUnique = guis[i]->interactive;
		if ( isUnique ) {
			unique++;
		} else {
			copies++;
		}
		common->Printf( "%6.1fk %4i (%s) %s ( %i transitions )\n", sz / 1024.0f, guis[i]->GetRefs(), isUnique ? "unique" : "copy", guis[i]->GetSourceFile(), guis[i]->desktop->NumTransitions() );
		total += sz;
	}
	common->Printf( "===========\n  %i total Guis ( %i copies, %i unique ), %.2f total Mbytes", c, copies, unique, total / ( 1024.0f * 1024.0f ) );
}

bool idUserInterfaceManagerLocal::CheckGui( const char *qpath ) const {
	idFile *file = fileSystem->OpenFileRead( qpath );
	if ( file ) {
		fileSystem->CloseFile( file );
		return true;
	}
	return false;
}

idUserInterface *idUserInterfaceManagerLocal::Alloc( void ) const {
	return new idUserInterfaceLocal();
}

void idUserInterfaceManagerLocal::DeAlloc( idUserInterface *gui ) {
	if ( gui ) {
		int c = guis.Num();
		for ( int i = 0; i < c; i++ ) {
			if ( guis[i] == gui ) {
				delete guis[i];
				guis.RemoveIndex( i );
				return;
			}
		}
	}
}

idUserInterface *idUserInterfaceManagerLocal::FindGui( const char *qpath, bool autoLoad, bool needUnique, bool forceNOTUnique ) {
	int c = guis.Num();

	for ( int i = 0; i < c; i++ ) {
		idUserInterfaceLocal *gui = guis[i];
		if ( !idStr::Icmp( guis[i]->GetSourceFile(), qpath ) ) {
			if ( !forceNOTUnique && ( needUnique || guis[i]->IsInteractive() ) ) {
				break;
			}
			guis[i]->AddRef();
			return guis[i];
		}
	}

	if ( autoLoad ) {
		idUserInterface *gui = Alloc();
		if ( gui->InitFromFile( qpath ) ) {
			gui->SetUniqued( forceNOTUnique ? false : needUnique );
			return gui;
		} else {
			delete gui;
		}
	}
	return NULL;
}

idUserInterface *idUserInterfaceManagerLocal::FindDemoGui( const char *qpath ) {
	int c = demoGuis.Num();
	for ( int i = 0; i < c; i++ ) {
		if ( !idStr::Icmp( demoGuis[i]->GetSourceFile(), qpath ) ) {
			return demoGuis[i];
		}
	}
	return NULL;
}

idListGUI *	idUserInterfaceManagerLocal::AllocListGUI( void ) const {
	return new idListGUILocal();
}

void idUserInterfaceManagerLocal::FreeListGUI( idListGUI *listgui ) {
	delete listgui;
}

/*
===============================================================================

	idUserInterfaceLocal

===============================================================================
*/

idUserInterfaceLocal::idUserInterfaceLocal() {
	cursorX = cursorY = 0.0;
	desktop = NULL;
	loading = false;
	active = false;
	interactive = false;
	uniqued = false;
	bindHandler = NULL;
	//so the reg eval in gui parsing doesn't get bogus values
	time = 0;
	refs = 1;
}

idUserInterfaceLocal::~idUserInterfaceLocal() {
	delete desktop;
	desktop = NULL;
}

const char *idUserInterfaceLocal::Name() const {
	return source;
}

const char *idUserInterfaceLocal::Comment() const {
	if ( desktop ) {
		return desktop->GetComment();
	}
	return "";
}

bool idUserInterfaceLocal::IsInteractive() const {
	return interactive;
}

bool idUserInterfaceLocal::InitFromFile( const char *qpath, bool rebuild, bool cache ) {
	if ( !( qpath && *qpath ) ) { 
		// FIXME: Memory leak!!
		return false;
	}


	idHashTable<idParser*> sourcePatchMap;
	InitPatchFiles( qpath, sourcePatchMap );

	loading = true;

	if ( rebuild ) {
		delete desktop;
		desktop = new idWindow( this );
	} else if ( desktop == NULL ) {
		desktop = new idWindow( this );
	}

	source = qpath;

	idParser src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );

	//Load the timestamp so reload guis will work correctly
	fileSystem->ReadFile(qpath, NULL, &timeStamp);

	src.LoadFile( qpath );

	if ( src.IsLoaded() ) {
		idToken token;
		while( src.ReadToken( &token ) ) {
			if ( idStr::Icmp( token, "windowDef" ) == 0 ) {

				idParser* originalSource = NULL;

				// Read the window identifier
				idToken windowName;
				src.ExpectTokenType( TT_NAME, 0, &windowName );
				idParser* source = &src;

				// See if there is a patch for the whole Window
				idParser** sourcePatch;
				if ( sourcePatchMap.Get( windowName, &sourcePatch ) ) {

					// Save the current source
					originalSource = source;

					// Replace the current source with the source from the patch.
					source = *sourcePatch;

					// We already know this defines an idWindow, so don't bother to validate
					source->ExpectAnyToken( &token );

					// The root Window is hard coded as a windowDef.  Make sure the patch is for the same.
					if ( idStr::Icmp( token, "windowDef" ) == 0 ) {

						// Skip the rest of this Window and ignore it.
						// OPTIONAL:  We might want to go back later and and parse this for the window names so
						//   we can explicitly include text from some sections.  We'll implement this change is it
						//   is demanded.
						// Original file must be properly formatted.
						if ( originalSource->SkipBracedSection() ) {

							// Can't reuse source patches
							sourcePatchMap.Remove( windowName );

							source->ExpectTokenType( TT_NAME, 0, &windowName );
						}

					} else {
						common->Warning( "The root window must be a windowDef.  Ignoring patch for '%s'.", windowName.c_str() );
						(*sourcePatch)->UnreadToken( &token );
					}
				}

				desktop->SetDC( &uiManagerLocal.dc );
				if ( desktop->Parse( source, sourcePatchMap, windowName, rebuild ) ) {
					desktop->SetFlag( WIN_DESKTOP );
					desktop->FixupParms();
				}

				// The original source was saved off.  Restore it.
				if ( originalSource != NULL ) {

					// Can't reuse source patches
					delete source;

					source = originalSource;
				}
				continue;
			}
		}

		state.Set( "name", qpath );
	} else {
		desktop->SetDC( &uiManagerLocal.dc );
		desktop->SetFlag( WIN_DESKTOP );
		desktop->name = "Desktop";
		desktop->text = va( "Invalid GUI: %s", qpath );
		desktop->rect = idRectangle( 0.0f, 0.0f, 640.0f, 480.0f );
		desktop->drawRect = desktop->rect;
		desktop->foreColor = idVec4( 1.0f, 1.0f, 1.0f, 1.0f );
		desktop->backColor = idVec4( 0.0f, 0.0f, 0.0f, 1.0f );
		desktop->SetupFromState();
		common->Warning( "Couldn't load gui: '%s'", qpath );
	}

	interactive = desktop->Interactive();

	if ( uiManagerLocal.guis.Find( this ) == NULL ) {
		uiManagerLocal.guis.Append( this );
	}

	loading = false;

	DeletePatchData( sourcePatchMap );

	return true; 
}

void idUserInterfaceLocal::PatchFromFile( const char *qpath ) {

	// TODO:  eviljoel:  Not sure if this check is needed.  Especially until we can have GUI patches initiated from
	//   user data.
	/*if ( qpath && *qpath ) {

		loading = true;

		source = qpath;
		//state.Set( "text", "Test Text!" );

		idParser src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );

		//Load the timestamp so reload guis will work correctly
		// TODO:  How does the timestamp work with GUI patches?
		fileSystem->ReadFile(qpath, NULL, &timeStamp);

		src.LoadFile( qpath );

		if ( src.IsLoaded() ) {
			idToken token;
			while( src.ReadToken( &token ) ) {
				// TODO:  Likely not just a windowDef
				if ( idStr::Icmp( token, "windowDef" ) == 0 ) {
					if ( desktop->Parse( &src, false ) ) {  // TODO:  Not sure if we can reuse Parse() yet.
						desktop->FixupParms();
					}
				}
			}

			//state.Set( "name", qpath );

		} else {
			// If a GUI patch does not load, ignore it and fall back to the default behavior.
			common->Warning( "Couldn't load GUI patch: '%s'", qpath );
		}

		loading = false;
	}*/
}

const char *idUserInterfaceLocal::HandleEvent( const sysEvent_t *event, int _time, bool *updateVisuals ) {

	time = _time;

	if ( bindHandler && event->evType == SE_KEY && event->evValue2 == 1 ) {
		const char *ret = bindHandler->HandleEvent( event, updateVisuals );
		bindHandler = NULL;
		return ret;
	}

	if ( event->evType == SE_MOUSE ) {
		cursorX += event->evValue;
		cursorY += event->evValue2;

		if (cursorX < 0) {
			cursorX = 0;
		}
		if (cursorY < 0) {
			cursorY = 0;
		}
	}

	if ( desktop ) {
		return desktop->HandleEvent( event, updateVisuals );
	} 

	return "";
}

void idUserInterfaceLocal::HandleNamedEvent ( const char* eventName ) {
	desktop->RunNamedEvent( eventName );
}

void idUserInterfaceLocal::Redraw( int _time ) {
	if ( r_skipGuiShaders.GetInteger() > 5 ) {
		return;
	}
	if ( !loading && desktop ) {
		time = _time;
		uiManagerLocal.dc.PushClipRect( uiManagerLocal.screenRect );
		desktop->Redraw( 0, 0 );
		uiManagerLocal.dc.PopClipRect();
	}
}

void idUserInterfaceLocal::DrawCursor() {
	if ( !desktop || desktop->GetFlags() & WIN_MENUGUI ) {
		uiManagerLocal.dc.DrawCursor(&cursorX, &cursorY, 32.0f );
	} else {
		uiManagerLocal.dc.DrawCursor(&cursorX, &cursorY, 64.0f );
	}
}

const idDict &idUserInterfaceLocal::State() const {
	return state;
}

void idUserInterfaceLocal::DeleteStateVar( const char *varName ) {
	state.Delete( varName );
}

void idUserInterfaceLocal::SetStateString( const char *varName, const char *value ) {
	state.Set( varName, value );
}

void idUserInterfaceLocal::SetStateBool( const char *varName, const bool value ) {
	state.SetBool( varName, value );
}

void idUserInterfaceLocal::SetStateInt( const char *varName, const int value ) {
	state.SetInt( varName, value );
}

void idUserInterfaceLocal::SetStateFloat( const char *varName, const float value ) {
	state.SetFloat( varName, value );
}

const char* idUserInterfaceLocal::GetStateString( const char *varName, const char* defaultString ) const {
	return state.GetString(varName, defaultString);
}

bool idUserInterfaceLocal::GetStateBool( const char *varName, const char* defaultString ) const {
	return state.GetBool(varName, defaultString); 
}

int idUserInterfaceLocal::GetStateInt( const char *varName, const char* defaultString ) const {
	return state.GetInt(varName, defaultString);
}

float idUserInterfaceLocal::GetStateFloat( const char *varName, const char* defaultString ) const {
	return state.GetFloat(varName, defaultString);
}

void idUserInterfaceLocal::StateChanged( int _time, bool redraw ) {
	time = _time;
	if (desktop) {
		desktop->StateChanged( redraw );
	}
	if ( state.GetBool( "noninteractive" ) ) {
		interactive = false;
	}
	else {
		if (desktop) {
			interactive = desktop->Interactive();
		} else {
			interactive = false;
		}
	}
}

const char *idUserInterfaceLocal::Activate(bool activate, int _time) {
	time = _time;
	active = activate;
	if ( desktop ) {
		activateStr = "";
		desktop->Activate( activate, activateStr );
		return activateStr;
	}
	return "";
}

void idUserInterfaceLocal::Trigger(int _time) {
	time = _time;
	if ( desktop ) {
		desktop->Trigger();
	}
}

void idUserInterfaceLocal::ReadFromDemoFile( class idDemoFile *f ) {
	idStr work;
	f->ReadDict( state );
	source = state.GetString("name");

	if (desktop == NULL) {
		f->Log("creating new gui\n");
		desktop = new idWindow(this);
	   	desktop->SetFlag( WIN_DESKTOP );
	   	desktop->SetDC( &uiManagerLocal.dc );
		desktop->ReadFromDemoFile(f);
	} else {
		f->Log("re-using gui\n");
		desktop->ReadFromDemoFile(f, false);
	}

	f->ReadFloat( cursorX );
	f->ReadFloat( cursorY );

	bool add = true;
	int c = uiManagerLocal.demoGuis.Num();
	for ( int i = 0; i < c; i++ ) {
		if ( uiManagerLocal.demoGuis[i] == this ) {
			add = false;
			break;
		}
	}

	if (add) {
		uiManagerLocal.demoGuis.Append(this);
	}
}

void idUserInterfaceLocal::WriteToDemoFile( class idDemoFile *f ) {
	idStr work;
	f->WriteDict( state );
	if (desktop) {
		desktop->WriteToDemoFile(f);
	}

	f->WriteFloat( cursorX );
	f->WriteFloat( cursorY );
}

bool idUserInterfaceLocal::WriteToSaveGame( idFile *savefile ) const {
	int len;
	const idKeyValue *kv;
	const char *string;

	int num = state.GetNumKeyVals();
	savefile->Write( &num, sizeof( num ) );

	for( int i = 0; i < num; i++ ) {
		kv = state.GetKeyVal( i );
		len = kv->GetKey().Length();
		string = kv->GetKey().c_str();
		savefile->Write( &len, sizeof( len ) );
		savefile->Write( string, len );

		len = kv->GetValue().Length();
		string = kv->GetValue().c_str();
		savefile->Write( &len, sizeof( len ) );
		savefile->Write( string, len );
	}

	savefile->Write( &active, sizeof( active ) );
	savefile->Write( &interactive, sizeof( interactive ) );
	savefile->Write( &uniqued, sizeof( uniqued ) );
	savefile->Write( &time, sizeof( time ) );
	len = activateStr.Length();
	savefile->Write( &len, sizeof( len ) );
	savefile->Write( activateStr.c_str(), len );
	len = pendingCmd.Length();
	savefile->Write( &len, sizeof( len ) );
	savefile->Write( pendingCmd.c_str(), len );
	len = returnCmd.Length();
	savefile->Write( &len, sizeof( len ) );
	savefile->Write( returnCmd.c_str(), len );

	savefile->Write( &cursorX, sizeof( cursorX ) );
	savefile->Write( &cursorY, sizeof( cursorY ) );

	desktop->WriteToSaveGame( savefile );

	return true;
}

bool idUserInterfaceLocal::ReadFromSaveGame( idFile *savefile ) {
	int num;
	int i, len;
	idStr key;
	idStr value;

	savefile->Read( &num, sizeof( num ) );

	state.Clear();
	for( i = 0; i < num; i++ ) {
		savefile->Read( &len, sizeof( len ) );
		key.Fill( ' ', len );
		savefile->Read( &key[0], len );

		savefile->Read( &len, sizeof( len ) );
		value.Fill( ' ', len );
		savefile->Read( &value[0], len );
		
		state.Set( key, value );
	}

	savefile->Read( &active, sizeof( active ) );
	savefile->Read( &interactive, sizeof( interactive ) );
	savefile->Read( &uniqued, sizeof( uniqued ) );
	savefile->Read( &time, sizeof( time ) );

	savefile->Read( &len, sizeof( len ) );
	activateStr.Fill( ' ', len );
	savefile->Read( &activateStr[0], len );
	savefile->Read( &len, sizeof( len ) );
	pendingCmd.Fill( ' ', len );
	savefile->Read( &pendingCmd[0], len );
	savefile->Read( &len, sizeof( len ) );
	returnCmd.Fill( ' ', len );
	savefile->Read( &returnCmd[0], len );

	savefile->Read( &cursorX, sizeof( cursorX ) );
	savefile->Read( &cursorY, sizeof( cursorY ) );

	desktop->ReadFromSaveGame( savefile );

	return true;
}

size_t idUserInterfaceLocal::Size() {
	size_t sz = sizeof(*this) + state.Size() + source.Allocated();
	if ( desktop ) {
		sz += desktop->Size();
	}
	return sz;
}

void idUserInterfaceLocal::RecurseSetKeyBindingNames( idWindow *window ) {
	int i;
	idWinVar *v = window->GetWinVarByName( "bind" );
	if ( v ) {
		SetStateString( v->GetName(), idKeyInput::KeysFromBinding( v->GetName() ) );
	}
	i = 0;
	while ( i < window->GetChildCount() ) {
		idWindow *next = window->GetChild( i );
		if ( next ) {
			RecurseSetKeyBindingNames( next );
		}
		i++;
	}
}

/*
==============
idUserInterfaceLocal::SetKeyBindingNames
==============
*/
void idUserInterfaceLocal::SetKeyBindingNames( void ) {
	if ( !desktop ) {
		return;
	}
	// walk the windows
	RecurseSetKeyBindingNames( desktop );
}

/*
==============
idUserInterfaceLocal::SetCursor
==============
*/
void idUserInterfaceLocal::SetCursor( float x, float y ) {
	cursorX = x;
	cursorY = y;
}

/*
==============
idUserInterfaceLocal::InitPatchFiles
==============
 */
void idUserInterfaceLocal::InitPatchFiles( const char* qpath, idHashTable<idParser*>& sourcePatchMap ) {

	idStrList** patchPaths = NULL;
	if ( uiManagerLocal.guiToPatchFilesMap.Get( qpath, &patchPaths ) ) {

		int patchPathCount = (*patchPaths)->Num();
		idStr patchSourceString;  // Defined out here to avoid object creation.
		for ( int patchPathIndex; patchPathIndex < patchPathCount; patchPathIndex++ ) {
			const char* patchPath = (**patchPaths)[patchPathIndex].c_str();

			// Load the timestamp so reload guis will work correctly
			fileSystem->ReadFile( patchPath, NULL, &timeStamp );

			idParser patchFileParser( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );
			patchFileParser.LoadFile( patchPath );

			if ( patchFileParser.IsLoaded() ) {
				idToken windowTypeToken;
				while( patchFileParser.ReadToken( &windowTypeToken ) ) {

					// Make sure the first token describes a window
					if ( idWindow::IsWindowToken(windowTypeToken) ) {

						// Error if the second token does not describe the Window
						idToken windowNameToken;
						if ( !patchFileParser.ExpectTokenType( TT_NAME, 0, &windowNameToken ) ) {
							common->Warning(
									"Window name expected while loading GUI patch source: '%s'", patchPath );
						} else {

							patchSourceString = windowTypeToken;
							patchSourceString.Append( " " );
							patchSourceString.Append( windowNameToken );
							patchSourceString.Append( " " );
							idStr discard;
							// TODO:  eviljoel:  The memory allocation within idStr is causing malloc errors with large patch files (and is slow).
							patchSourceString.Append( patchFileParser.ParseBracedSection( discard, 0 ) );

							// LoadMemory below expects a null terminated C string.
							const int patchSourceLength = patchSourceString.Length() + 1;
							char* patchSource = Mem_Alloc( patchSourceLength );
							idStr::Copynz( patchSource, patchSourceString.c_str(), patchSourceLength );

							idParser* patchSourceParser =
									new idParser( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );
							// TODO:  eviljoel:  idParser mostly just uses the filename for error messages.  (It is
							//   also passed it on to idLexer.)  Maybe we should pass another parameter explaining what
							//   part of the patch file we are working with.
							patchSourceParser->LoadMemory( patchSource, patchSourceLength - 1, patchPath );

							// Erase the existing patch if applicable
							idParser** existingPatch;
							if ( sourcePatchMap.Get( windowNameToken, &existingPatch ) ) {
								delete *existingPatch;
							}

							sourcePatchMap.Set( windowNameToken, patchSourceParser );
						}
					}
				}
			} else {
				common->Warning( "Couldn't load gui patch: '%s'", patchPath );
			}
		}
	}
}

/*
==============
idUserInterfaceLocal::DeletePatchData
==============
 */
void idUserInterfaceLocal::DeletePatchData( idHashTable<idParser*>& sourcePatchMap ) {

	const int sourcePatchCount = sourcePatchMap.Num();
	for ( int sourcePatchIndex = 0; sourcePatchIndex < sourcePatchCount; sourcePatchIndex++ ) {
		idParser** sourcePatch = sourcePatchMap.GetIndex( sourcePatchIndex );
		delete *sourcePatch;
	}

	sourcePatchMap.Clear();
}
