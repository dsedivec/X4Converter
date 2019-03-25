#include <X4ConverterTools/Xmf/XmfHeader.h>

using namespace Assimp;
using namespace boost;

XmfHeader::XmfHeader(Assimp::StreamReader<> &reader) {
    for (byte& c : Magic) {
        reader >> c;
    }
    reader >> Version >> BigEndian;
    reader >> DataBufferDescOffset;
    reader >> _pad0;
    reader >> NumDataBuffers;
    reader >> DataBufferDescSize;
    reader >> NumMaterials;
    reader >> MaterialSize;

    for (byte& c : _pad1) {
        reader >> c;
    }
    reader >> PrimitiveType;
    for (byte& c : _pad2) {
        reader >> c;
    }
}

std::string XmfHeader::validate() const {
    std::string ret;


    if (memcmp(Magic, "XUMF\x03", 5) != 0) {
        throw std::runtime_error("Invalid header magic");
    }
    if (BigEndian) {
        throw std::runtime_error(
                "Big endian .xmf files are not supported by this importer");
    }
    if (_pad0 != 0) {
        throw std::runtime_error(str(format("padding0 should be 0, was %1%") % _pad0));
    }
    if (DataBufferDescOffset != XmfHeader::BUFFER_OFFSET) {
        std::cout << DataBufferDescOffset << std::endl;
        throw std::runtime_error("Offset should be 0x40");
    }
    if (DataBufferDescSize > sizeof(XmfDataBufferDesc)) {
        throw std::runtime_error("Data buffer description size is too large");
    }
    if (MaterialSize != sizeof(XmfMaterial)) {
        throw std::runtime_error("Material size is invalid");
    }

    // TODO  this and pad 2
//    for (int i = 0; i < 10; ++i) {
//        if (_pad1[i] != 0){
//            throw std::runtime_error("padding should be 0!");
//        }
//    }
    if (PrimitiveType != D3DPT_TRIANGLELIST) {
        throw std::runtime_error(
                "File is using a DirectX primitive type that's not supported by this importer");
    }
    return ret;
}

void XmfHeader::Write(Assimp::StreamWriter<> &writer) {

    for (byte& c : Magic) {
        writer << c;
    }
    writer << Version << BigEndian;
    writer << DataBufferDescOffset;
    writer << _pad0;
    writer << NumDataBuffers;
    writer << DataBufferDescSize;
    writer << NumMaterials;
    writer << MaterialSize;

    for (byte& c : _pad1) {
        writer << c;
    }
    writer << PrimitiveType;

    for (byte& c : _pad2) {
        writer << c;
    }
}


XmfHeader::XmfHeader(byte numDataBuffers, byte numMaterials) {
// Note that this ignores any header read into this mesh file since we can't guarantee the old header is accurate
    Magic[0] = 'X';
    Magic[1] = 'U';
    Magic[2] = 'M';
    Magic[3] = 'F';
    Version = 3;
    BigEndian = false;
    DataBufferDescOffset = 0x40;
    _pad0 = 0x00;
    NumDataBuffers = numDataBuffers;
    DataBufferDescSize = sizeof(XmfDataBufferDesc);
    NumMaterials = numMaterials;
    MaterialSize = sizeof(XmfMaterial);
    for (byte& c : _pad1) {
        c = 0x00;
    }
    PrimitiveType = D3DPT_TRIANGLELIST;
    for (byte& c : _pad2) {
        c = 0x00;
    }

}