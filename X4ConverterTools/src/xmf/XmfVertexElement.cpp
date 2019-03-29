#include <X4ConverterTools/xmf/XmfVertexElement.h>
namespace xmf {
    XmfVertexElement::XmfVertexElement() {
        Type = D3DDECLTYPE_UNUSED;
    }

    XmfVertexElement::XmfVertexElement(D3DDECLTYPE type, D3DDECLUSAGE usage, uint8_t usageIndex) {
        Type = type;
        Usage = usage;
        UsageIndex = usageIndex;
    }

    XmfVertexElement::XmfVertexElement(Assimp::StreamReaderLE &reader) {
        reader >> Type;
        reader >> Usage;
        reader >> UsageIndex;
        reader >> _pad0[0] >> _pad0[1];
// TODO check padding and ask about it
//    if (_pad0[0]!=0 || _pad0[1] !=0){
//        std::cout << str(format("%1% %2%") % (int)_pad0[0] % (int)_pad0[1])<<std::endl;
//        throw std::runtime_error("_pad0 must be 0!");
//    }
    }

    void XmfVertexElement::Write(Assimp::StreamWriterLE &writer) {
        writer << Type;
        writer << Usage;
        writer << UsageIndex;
        writer << _pad0[0] << _pad0[1];
    }

}