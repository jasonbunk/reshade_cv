// Copyright (C) 2023 Jason Bunk
#include "customize_dxbc.hpp"

#ifndef RENDERDOC_FOR_SHADERS

bool customize_shader_dxbc_or_dxil(
	bool b_truepixel_falsevertex,
	const reshade::api::device_api graphics_api,
	bytebuf* buf,
	custom_shader_layout_registers& newregisters,
	std::stringstream& log)
{
	log << "NOT IMPLEMENTED: customize_shader_dxbc_or_dxil" << std::endl;
	return false;
}

#else

// you may want to #define: RENDERDOC_FOR_SHADERS;RENDERDOC_EXPORTS;RENDERDOC_PLATFORM_WIN32;
#include "rdoc_utils.hpp"
#include "driver/shaders/dxbc/dxbc_container.h" // renderdoc header
#include "driver/shaders/dxbc/dxbc_bytecode_editor.h" // renderdoc header
#include <unordered_set>
#include <d3d10_1.h>
#include <d3d11.h>
#include <d3d12.h>
static constexpr int verbose = 0;
using std::endl;
using reshade::api::device_api;

static int highest_decl_register(std::unordered_set<DXBCBytecode::OpcodeType> considered_reg_op_types, DXBCBytecode::Program* prog, std::stringstream& log) {
	int maxtexreg = -1;
	for (size_t ii = 0; ii < prog->GetNumDeclarations(); ++ii) {
		DXBCBytecode::Declaration iodec = prog->GetDeclaration(ii);
		if (considered_reg_op_types.count(iodec.declaration)) {
			if (iodec.operand.indices.size() != 1) {
				log << "weird decl with more than one operand index: " << iodec.operand.name.c_str() << ": " << static_cast<int>(iodec.operand.type) << ": " << iodec.operand.indices.size() << endl;
			}
			if (iodec.operand.indices.size() >= 1) {
				maxtexreg = std::max<int>(maxtexreg, iodec.operand.indices.at(0).index);
			}
		}
	}
	return maxtexreg;
}

static int get_num_decl_temps(DXBCBytecode::Program* prog) {
	for (size_t ii = 0; ii < prog->GetNumDeclarations(); ++ii) {
		DXBCBytecode::Declaration iodec = prog->GetDeclaration(ii);
		if (iodec.declaration == DXBCBytecode::OPCODE_DCL_TEMPS) {
			return iodec.numTemps;
		}
	}
	return -1;
}

static DXBCBytecode::Operand new_operand(DXBCBytecode::OperandType type, int reg) {
	DXBCBytecode::Operand newo;
	newo.type = type;
	newo.indices.resize(1);
	newo.indices[0].absolute = 1;
	newo.indices[0].index = reg;
	return newo;
}

static DXBCBytecode::Operand new_operand_with_value_no_index(DXBCBytecode::NumOperandComponents nc, uint32_t x, uint32_t y, uint32_t z, uint32_t w) {
	DXBCBytecode::Operand newo;
	newo.type = DXBCBytecode::TYPE_IMMEDIATE32;
	newo.numComponents = nc;
	newo.flags = DXBCBytecode::Operand::FLAG_MASKED;
	newo.comps[0] = newo.comps[1] = newo.comps[2] = newo.comps[3] = 0xff;
	newo.values[0] = x;
	newo.values[1] = y;
	newo.values[2] = z;
	newo.values[3] = w;
	return newo;
}

// Input declarations will need more description than is set here: e.g. either specifying semantic, or resource details
static DXBCBytecode::Declaration new_decl(DXBCBytecode::OpcodeType opc_type, DXBCBytecode::Operand opernd) {
	DXBCBytecode::Declaration newd;
	newd.declaration = opc_type;
	newd.operand = opernd;
	return newd;
}

static DXBCBytecode::Operation new_op_mov(DXBCBytecode::Operand dst, DXBCBytecode::Operand src) {
	DXBCBytecode::Operation mov;
	mov.operation = DXBCBytecode::OPCODE_MOV;
	mov.operands.resize(2);
	mov.operands[0] = dst;
	mov.operands[1] = src;
	return mov;
}

bool customize_dxbc(bool b_truepixel_falsevertex, const device_api graphics_api, DXBC::DXBCContainer & replayvsc, bytebuf* buf, custom_shader_layout_registers& newregisters, std::stringstream& log) {
	if (buf == nullptr) {
		log << "?? customize_dxbc: no buf to write to" << endl;
		return false;
	}
	if (replayvsc.GetDXBCByteCode() == nullptr) {
		log << "  renderdoc GetDXBCByteCode() null!" << endl;
		return false;
	}
	const DXBC::Reflection* reflection = replayvsc.GetReflection();
	if (reflection == nullptr) {
		log << "  renderdoc GetReflection() null!" << endl;
		return false;
	}
	log << "DXBC shader model detected: v" << replayvsc.GetDXBCByteCode()->GetMajorVersion() << "." << replayvsc.GetDXBCByteCode()->GetMinorVersion() << endl;
	DXBCBytecode::ProgramEditor shadereditor(&replayvsc, *buf);
	// print shader info
	if (verbose) {
		log << "  renderdoc_shader_disassembly: " << shadereditor.GetNumInstructions() << " instructions, " << shadereditor.GetNumDeclarations() << " declarations: "
			<< static_cast<int>(shadereditor.GetShaderType()) << ":" << shadereditor.GetMajorVersion() << "." << shadereditor.GetMinorVersion() << endl;
		if (verbose > 1) { const rdcstr astr = shadereditor.GetDisassembly(); log << std::string(astr.begin(), astr.end()) << endl; }
	}
	// print output signature
	if (reflection->OutputSig.empty()) {
		if (verbose > 1) log << "  shadersigparam: no semantic outputs" << endl;
	}
	else
		for (const SigParameter& sign : reflection->OutputSig) {
			if (verbose > 1) log << "  shadersigparam: semanticIdx: " << std::to_string(sign.semanticIndex)
				<< ", semanticName: " << std::string(sign.semanticName.c_str()) << endl;
		}
	if (shadereditor.GetNumInstructions() <= 1 || shadereditor.GetNumDeclarations() == 0) {
		log << "bad num instructions or declarations: #ins " << shadereditor.GetNumInstructions() << " or #dec " << shadereditor.GetNumDeclarations() << endl;
		{ const rdcstr astr = shadereditor.GetDisassembly(); log << std::string(astr.begin(), astr.end()) << endl; }
		return false;
	}

	const uint64_t max_num_bound_pixel_shader_outputs =
		(graphics_api == device_api::d3d10 ? D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT :
		 (graphics_api == device_api::d3d11 ? D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT :
		  (graphics_api == device_api::d3d12 ? D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT : 8)));

	// count number of declared outputs
	const int num_decl_outputs = 1 + highest_decl_register({ DXBCBytecode::OPCODE_DCL_OUTPUT, DXBCBytecode::OPCODE_DCL_OUTPUT_SGV, DXBCBytecode::OPCODE_DCL_OUTPUT_SIV }, &shadereditor, log);
	if (num_decl_outputs <= 0 || (b_truepixel_falsevertex && num_decl_outputs >= max_num_bound_pixel_shader_outputs)) {
		log << "bad number of declared outputs " << num_decl_outputs << ": shader with #ins " << shadereditor.GetNumInstructions() << " or #dec " << shadereditor.GetNumDeclarations() << endl;
		{ const rdcstr astr = shadereditor.GetDisassembly(); log << std::string(astr.begin(), astr.end()) << endl; }
		return false;
	}

	// count number of declared inputs
	const int num_decl_inputs = 1 + (
		b_truepixel_falsevertex ? highest_decl_register({ DXBCBytecode::OPCODE_DCL_INPUT_PS, DXBCBytecode::OPCODE_DCL_INPUT_PS_SGV, DXBCBytecode::OPCODE_DCL_INPUT_PS_SIV }, &shadereditor, log)
	                            : highest_decl_register({ DXBCBytecode::OPCODE_DCL_INPUT,    DXBCBytecode::OPCODE_DCL_INPUT_SGV,    DXBCBytecode::OPCODE_DCL_INPUT_SIV    }, &shadereditor, log) );
	if (num_decl_inputs <= 0) {
		log << "bad highest declaration input register " << num_decl_inputs << ": shader with #ins " << shadereditor.GetNumInstructions() << " or #dec " << shadereditor.GetNumDeclarations() << endl;
		{ const rdcstr astr = shadereditor.GetDisassembly(); log << std::string(astr.begin(), astr.end()) << endl; }
		return false;
	}

	// count number of declared texture resources
	const int num_decl_textures = 1 + highest_decl_register({ DXBCBytecode::OPCODE_DCL_RESOURCE, DXBCBytecode::OPCODE_DCL_RESOURCE_RAW, DXBCBytecode::OPCODE_DCL_RESOURCE_STRUCTURED }, &shadereditor, log);

	const int num_decl_temps = get_num_decl_temps(&shadereditor);

	// now start creating new declaration(s) and new instruction(s)

	std::vector<std::pair<DXBCBytecode::Declaration, DXBCBytecode::OpcodeType>> new_decls;
	std::vector<DXBCBytecode::Operation> new_insts;

	if (!b_truepixel_falsevertex) {
		// vertex shader : may not need input or output register(s) if was already tracking SV_InstanceID... SM40/DX10 said it's okay to repeat, but IDK about SM51+ or DX11+
		const int new_shader_out_reg = std::max(num_decl_outputs, 15);

		// declare new input for SV_InstanceID
		new_decls.emplace_back(new_decl(DXBCBytecode::OPCODE_DCL_INPUT_SGV, new_operand(DXBCBytecode::TYPE_INPUT, num_decl_inputs).swizzle(0, 0xff, 0xff, 0xff)), DXBCBytecode::OPCODE_DCL_INPUT_SIV);
		new_decls.back().first.inputOutput.systemValue = DXBC::SVNAME_INSTANCE_ID;

		// declare new output to pass along InstanceID
		new_decls.emplace_back(new_decl(DXBCBytecode::OPCODE_DCL_OUTPUT, new_operand(DXBCBytecode::TYPE_OUTPUT, new_shader_out_reg).swizzle(0, 0xff, 0xff, 0xff)), DXBCBytecode::OPCODE_DCL_OUTPUT_SIV);

		// instruction to mov InstanceID from input to output
		new_insts.push_back(new_op_mov(
			new_operand(DXBCBytecode::TYPE_OUTPUT, new_shader_out_reg).swizzle(0, 0xff, 0xff, 0xff), // swizzle(0,0xff,0xff,0xff) --> FLAG_MASKED   = 32, numComponents = NUMCOMPS_4
			new_operand(DXBCBytecode::TYPE_INPUT,  num_decl_inputs ).swizzle(0)                      // swizzle(0)                --> FLAG_SELECTED =  8, numComponents = NUMCOMPS_4
		));

	}
	else {
		// pixel shader
		const int new_shader_in_reg = std::max(num_decl_inputs, 15);

		// declare my new per-draw buffer resource
		{
			DXBCBytecode::Declaration texdec = new_decl(DXBCBytecode::OPCODE_DCL_RESOURCE, new_operand(DXBCBytecode::TYPE_RESOURCE, num_decl_textures));
			if (shadereditor.IsShaderModel51()) {
				texdec.operand.numComponents = DXBCBytecode::NUMCOMPS_4;
				texdec.operand.swizzle(0, 1, 2, 3);
				//texdec.operand.indices.push_back(texdec.operand.indices.front());
				//texdec.operand.indices.push_back(texdec.operand.indices.front());
			} else {
				texdec.operand.numComponents = DXBCBytecode::NUMCOMPS_0;
				texdec.operand.swizzle(0xff, 0xff, 0xff, 0xff);
			}
			texdec.resource.resType[0] = texdec.resource.resType[1] = texdec.resource.resType[2] = texdec.resource.resType[3] = DXBC::RETURN_TYPE_UINT;
			texdec.resource.dim = DXBCBytecode::RESOURCE_DIMENSION_BUFFER;
			texdec.resource.sampleCount = 0;
			shadereditor.AppendDeclaration(texdec, DXBCBytecode::OPCODE_DCL_RESOURCE_STRUCTURED);
		}
		newregisters.perdrawbuf_tex_regL = num_decl_textures;
		newregisters.perdrawbuf_tex_regH = 0xff; // not used in ps_5_0

		// declare new input for passed along InstanceID, stored in the first component of the new input register
		new_decls.emplace_back(new_decl(DXBCBytecode::OPCODE_DCL_INPUT_PS, new_operand(DXBCBytecode::TYPE_INPUT, new_shader_in_reg).swizzle(0, 0xff, 0xff, 0xff)), DXBCBytecode::OPCODE_DCL_INPUT_PS_SIV);
		new_decls.back().first.inputOutput.inputInterpolation = DXBCBytecode::INTERPOLATION_CONSTANT;
		new_decls.back().first.inputOutput.systemValue = DXBC::SVNAME_INSTANCE_ID;

		// declare new input for PrimitiveID, stored in the second component of the new input register
		new_decls.emplace_back(new_decl(DXBCBytecode::OPCODE_DCL_INPUT_PS_SGV, new_operand(DXBCBytecode::TYPE_INPUT, new_shader_in_reg).swizzle(1, 0xff, 0xff, 0xff)), DXBCBytecode::OPCODE_DCL_INPUT_PS_SIV);
		new_decls.back().first.inputOutput.inputInterpolation = DXBCBytecode::INTERPOLATION_CONSTANT;
		new_decls.back().first.inputOutput.systemValue = DXBC::SVNAME_PRIMITIVE_ID;

		// declare new output for my new render target
		new_decls.emplace_back(new_decl(DXBCBytecode::OPCODE_DCL_OUTPUT, new_operand(DXBCBytecode::TYPE_OUTPUT, num_decl_outputs).swizzle(0, 1, 2, 3)), DXBCBytecode::OPCODE_DCL_OUTPUT_SIV);
		new_decls.back().first.operand.flags = DXBCBytecode::Operand::FLAG_MASKED;
		newregisters.rendertarget_index = num_decl_outputs;

		// declare new temp for loading from my new per-draw buffer
		const uint32_t newtmp = num_decl_temps > 0 ? 0 : shadereditor.AddTemp();

		// one "ld_indexable" instruction to load from my new per-draw buffer into the new temp
		DXBCBytecode::Operation& ldi = new_insts.emplace_back();
		ldi.operation = DXBCBytecode::OPCODE_LD;
		ldi.resDim = DXBCBytecode::RESOURCE_DIMENSION_BUFFER;
		ldi.resType[0] = ldi.resType[1] = ldi.resType[2] = ldi.resType[3] = DXBC::RETURN_TYPE_UINT;
		ldi.flags = DXBCBytecode::Operation::Flags(DXBCBytecode::Operation::FLAG_RESOURCE_DIMS | DXBCBytecode::Operation::FLAG_RET_TYPE);
		ldi.operands.resize(3);
		ldi.operands[0] = new_operand(DXBCBytecode::TYPE_TEMP, newtmp).swizzle(0, 0xff, 0xff, 0xff);// dst... might be swizzle(0,1,2,3)
		ldi.operands[1] = new_operand_with_value_no_index(DXBCBytecode::NUMCOMPS_4, 0, 0, 0, 0);    // idx
		ldi.operands[2] = new_operand(DXBCBytecode::TYPE_RESOURCE, newregisters.perdrawbuf_tex_regL).swizzle(0, 1, 2, 3);// buf resource
		ldi.operands[2].flags = DXBCBytecode::Operand::FLAG_SWIZZLED;
		ldi.operands[2].declaration = const_cast<DXBCBytecode::Declaration*>(shadereditor.FindDeclaration(DXBCBytecode::TYPE_RESOURCE, newregisters.perdrawbuf_tex_regL));
		if (ldi.operands[2].declaration == NULL) {
			log << "ERROR declaring resource at register t" << newregisters.perdrawbuf_tex_regL << endl;
			return false;
		}

		// four "mov" instructions to copy/write each component of output render target
		new_insts.push_back(new_op_mov(
			new_operand(DXBCBytecode::TYPE_OUTPUT, num_decl_outputs).swizzle(0, 0xff, 0xff, 0xff),
			new_operand(DXBCBytecode::TYPE_TEMP, newtmp).swizzle(0) // we just loaded new tmp with a read from our per-draw buffer
		));

		new_insts.push_back(new_op_mov(
			new_operand(DXBCBytecode::TYPE_OUTPUT, num_decl_outputs).swizzle(1, 0xff, 0xff, 0xff),
			new_operand(DXBCBytecode::TYPE_INPUT, new_shader_in_reg).swizzle(0) // InstanceID: first component of the new input
		));

		new_insts.push_back(new_op_mov(
			new_operand(DXBCBytecode::TYPE_OUTPUT, num_decl_outputs).swizzle(2, 0xff, 0xff, 0xff),
			new_operand(DXBCBytecode::TYPE_INPUT, new_shader_in_reg).swizzle(1) // PrimitiveID: second component of the new input
		));

		new_insts.push_back(new_op_mov(
			new_operand(DXBCBytecode::TYPE_OUTPUT, num_decl_outputs).swizzle(3, 0xff, 0xff, 0xff),
			new_operand_with_value_no_index(DXBCBytecode::NUMCOMPS_1, 1, 0, 0, 0) // constant value of 1
		));
	}

	for (const auto& x : new_decls) {
		shadereditor.AppendDeclaration(x.first, x.second);
	}
	for (const auto& x : new_insts) {
		shadereditor.InsertOperation(shadereditor.GetNumInstructions() - 1, x);
	}
	return true;
}


bool customize_shader_dxbc_or_dxil(
		bool b_truepixel_falsevertex,
		const device_api graphics_api,
		bytebuf* buf,
		custom_shader_layout_registers& newregisters,
		std::stringstream& log) {
	DXBC::DXBCContainer replayvsc(*buf, rdcstr(), reshade_api_to_renderdoc_api(graphics_api), ~0U, ~0U);
	if (replayvsc.CheckForDXIL(buf->data(), buf->size())) {
		log << "TODO: customize DXIL" << endl;
		return false;
	}
	return customize_dxbc(b_truepixel_falsevertex, graphics_api, replayvsc, buf, newregisters, log);
}
#endif
