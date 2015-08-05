#pragma once
#include <endianness.h>

namespace cg
{
	using namespace endianness;

	typedef be<u32> CGprofile;
	typedef be<s32> CGbool;
	typedef be<u32> CGresource;
	typedef be<u32> CGenum;
	typedef be<u32> CGtype;

	typedef be<u32>                         CgBinaryOffset;
	typedef CgBinaryOffset                  CgBinaryEmbeddedConstantOffset;
	typedef CgBinaryOffset                  CgBinaryFloatOffset;
	typedef CgBinaryOffset                  CgBinaryStringOffset;
	typedef CgBinaryOffset                  CgBinaryParameterOffset;

	// fragment programs have their constants embedded in the microcode
	struct CgBinaryEmbeddedConstant
	{
		be<u32> ucodeCount;       // occurances
		be<u32> ucodeOffset[1];   // offsets that need to be patched follow
	};

	// describe a binary program parameter (CgParameter is opaque)
	struct CgBinaryParameter
	{
		CGtype                          type;          // cgGetParameterType()
		CGresource                      res;           // cgGetParameterResource()
		CGenum                          var;           // cgGetParameterVariability()
		be<s32>                         resIndex;      // cgGetParameterResourceIndex()
		CgBinaryStringOffset            name;          // cgGetParameterName()
		CgBinaryFloatOffset             defaultValue;  // default constant value
		CgBinaryEmbeddedConstantOffset  embeddedConst; // embedded constant information
		CgBinaryStringOffset            semantic;      // cgGetParameterSemantic()
		CGenum                          direction;     // cgGetParameterDirection()
		be<s32>                         paramno;       // 0..n: cgGetParameterIndex() -1: globals
		CGbool                          isReferenced;  // cgIsParameterReferenced()
		CGbool                          isShared;	   // cgIsParameterShared()
	};

	// attributes needed for vshaders
	struct CgBinaryVertexProgram
	{
		be<u32>  instructionCount;         // #instructions
		be<u32>  instructionSlot;          // load address (indexed reads!)
		be<u32>  registerCount;            // R registers count
		be<u32>  attributeInputMask;       // attributes vs reads from
		be<u32>  attributeOutputMask;      // attributes vs writes (uses SET_VERTEX_ATTRIB_OUTPUT_MASK bits)
		be<u32>  userClipMask;             // user clip plane enables (for SET_USER_CLIP_PLANE_CONTROL)
	};

	typedef enum
	{
		CgBinaryPTTNone = 0,
		CgBinaryPTT2x16 = 1,
		CgBinaryPTT1x32 = 2
	} CgBinaryPartialTexType;

	// attributes needed for pshaders
	struct CgBinaryFragmentProgram
	{
		be<u32> instructionCount;        // #instructions
		be<u32> attributeInputMask;      // attributes fp reads (uses SET_VERTEX_ATTRIB_OUTPUT_MASK bits)
		be<u32> partialTexType;          // texid 0..15 use two bits each marking whether the texture format requires partial load: see CgBinaryPartialTexType
		be<u16> texCoordsInputMask;      // tex coords used by frag prog. (tex<n> is bit n)
		be<u16> texCoords2D;             // tex coords that are 2d        (tex<n> is bit n)
		be<u16> texCoordsCentroid;       // tex coords that are centroid  (tex<n> is bit n)
		u8      registerCount;           // R registers count
		u8      outputFromH0;            // final color from R0 or H0
		u8      depthReplace;            // fp generated z epth value
		u8      pixelKill;               // fp uses kill operations
	};

	struct CgBinaryProgram
	{
		// vertex/pixel shader identification (BE/LE as well)
		CGprofile profile;

		// binary revision (used to verify binary and driver structs match)
		be<u32> binaryFormatRevision;

		// total size of this struct including profile and totalSize field
		be<u32> totalSize;

		// parameter usually queried using cgGet[First/Next]LeafParameter
		be<u32> parameterCount;
		CgBinaryParameterOffset parameterArray;

		// depending on profile points to a CgBinaryVertexProgram or CgBinaryFragmentProgram struct
		CgBinaryOffset program;

		// raw ucode data
		be<u32>    ucodeSize;
		CgBinaryOffset  ucode;

		// variable length data follows
		u8 data[1];
	};
}