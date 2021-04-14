#include <pybind11/stl.h>
#include <pybind11/pybind11.h>

#include "libalac/ALACAudioTypes.h"
#include "libalac/ALACBitUtilities.h"
#include "libalac/ALACEncoder.h"
#include "libalac/ALACDecoder.h"

#include "libaes/aes_utils.h"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;


class AESEncrypter {
    private:
        uint8_t *key, *iv;
    public:

    AESEncrypter(py::bytes key, py::bytes iv) {
        int32_t size;
        // The aes key
        py::buffer_info key_info(py::buffer(key).request());
        size = static_cast<int32_t>(key_info.shape[0] * key_info.itemsize);
        this->key = (uint8_t *)malloc(size);
        memcpy(this->key, key_info.ptr, size);
        // The aes IV
        py::buffer_info iv_info(py::buffer(iv).request());
        size = static_cast<int32_t>(iv_info.shape[0] * iv_info.itemsize);
        this->iv = (uint8_t *)malloc(size);
        memcpy(this->iv, iv_info.ptr, size);
    }

    py::bytes encrypt(py::bytes data, int32_t block_size) {
        // input data as unsigned char* with the size in bytes
        py::buffer_info info(py::buffer(data).request());
        int32_t size = static_cast<int32_t>(info.shape[0] * info.itemsize);
        // target encrypted data
        std::string dst_string(size, 0);
        memcpy(&dst_string[0], info.ptr, size);

        uint8_t *buf;
        int i = 0, j;
        uint8_t *nv = new uint8_t[block_size];

        aes_context ctx;
        aes_set_key(&ctx, key, 128);
        memcpy(nv, iv, block_size);

        while(i + block_size <= size) {
            buf = (uint8_t *)&dst_string[0] + i;

            for(j = 0; j < block_size; j++)
              buf[j] ^= nv[j];

            aes_encrypt(&ctx, buf, buf);
            memcpy(nv, buf, block_size);

            i += block_size;
        }
        return py::bytes(dst_string);
    }

    ~AESEncrypter() {
        free(this->key);
        free(this->iv);
    }

};


enum ALACFormat {
    FormatAppleLossless = kALACFormatAppleLossless,
    FormatLinearPCM = kALACFormatLinearPCM
};

enum ALACFormatFlag {
    FormatFlagIsFloat = kALACFormatFlagIsFloat,
    FormatFlagIsBigEndian = kALACFormatFlagIsBigEndian,
    FormatFlagIsSignedInteger = kALACFormatFlagIsSignedInteger,
    FormatFlagIsPacked = kALACFormatFlagIsPacked,
    FormatFlagIsAlignedHigh = kALACFormatFlagIsAlignedHigh
};

PYBIND11_MODULE(libalac, m) {
    m.doc() = "Python libalac bindings.";


    py::class_<AESEncrypter>(m, "AESEncrypter")
        .def(py::init<py::bytes, py::bytes>())
        .def("encrypt", &AESEncrypter::encrypt, "Encyrpt data with an aes key.");


    py::enum_<ALACFormat>(m, "ALACFormat")
        .value("AppleLossless", FormatAppleLossless)
        .value("LinearPCM", FormatLinearPCM);

    py::enum_<ALACFormatFlag>(m, "ALACFormatFlag")
        .value("IsFloat", FormatFlagIsFloat)
        .value("IsBigEndian", FormatFlagIsBigEndian)
        .value("IsSignedInteger", FormatFlagIsSignedInteger)
        .value("IsPacked", FormatFlagIsPacked)
        .value("IsAlignedHigh", FormatFlagIsAlignedHigh);

    py::class_<AudioFormatDescription>(m, "AudioFormatDescription")
        .def(py::init<alac_float64_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>(),
             py::arg("sampleRate") = 44100.0,
             py::arg("format") = FormatAppleLossless,
             py::arg("formatFlags") = FormatLinearPCM,
             py::arg("bytesPerPacket") = 0,
             py::arg("framesPerPacket") = 352,
             py::arg("bytesPerFrame") = 0,
             py::arg("channelsPerFrame") = 2,
             py::arg("bitsPerChannel") = 0)
        .def_readwrite("sampleRate", &AudioFormatDescription::mSampleRate)
        .def_readwrite("formatID", &AudioFormatDescription::mFormatID)
        .def_readwrite("formatFlags", &AudioFormatDescription::mFormatFlags)
        .def_readwrite("bytesPerPacket", &AudioFormatDescription::mBytesPerPacket)
        .def_readwrite("framesPerPacket", &AudioFormatDescription::mFramesPerPacket)
        .def_readwrite("bytesPerFrame", &AudioFormatDescription::mBytesPerFrame)
        .def_readwrite("channelsPerFrame", &AudioFormatDescription::mChannelsPerFrame)
        .def_readwrite("bitsPerChannel", &AudioFormatDescription::mBitsPerChannel)
        .def_readwrite("reserved", &AudioFormatDescription::mReserved);

    py::class_<ALACSpecificConfig>(m, "ALACSpecificConfig")
        .def_readwrite("frameLength", &ALACSpecificConfig::frameLength)
        .def_readwrite("compatibleVersion", &ALACSpecificConfig::compatibleVersion)
        .def_readwrite("bitDepth", &ALACSpecificConfig::bitDepth)
        .def_readwrite("pb", &ALACSpecificConfig::pb)
        .def_readwrite("mb", &ALACSpecificConfig::mb)
        .def_readwrite("kb", &ALACSpecificConfig::kb)
        .def_readwrite("numChannels", &ALACSpecificConfig::numChannels)
        .def_readwrite("maxRun", &ALACSpecificConfig::maxRun)
        .def_readwrite("maxFrameBytes", &ALACSpecificConfig::maxFrameBytes)
        .def_readwrite("avgBitRate", &ALACSpecificConfig::avgBitRate)
        .def_readwrite("sampleRate", &ALACSpecificConfig::sampleRate);

    py::class_<ALACEncoder>(m, "ALACEncoder")
        .def(py::init([](AudioFormatDescription &output_format, int frames_size, bool fast_mode)
        {
            ALACEncoder *encoder = new ALACEncoder();
            encoder->SetFastMode(fast_mode);
            encoder->SetFrameSize(frames_size);
            encoder->InitializeEncoder(output_format);
            return encoder;
        }), py::arg("out_format"), py::arg("frame_size"), py::arg("fast_mode")=false)
        .def_property("config", [](ALACEncoder *encoder)
        {
            ALACSpecificConfig config;
            encoder->GetConfig(config);
            return config;
        }, nullptr, "Get the current ALAC configuration.")
        .def("get_magic_cookie", [](ALACEncoder *encoder, uint32_t num_channels)
        {
            uint32_t cookieSize = encoder->GetMagicCookieSize(num_channels);
            std::string cookiePtr(cookieSize, 0);
            encoder->GetMagicCookie(&cookiePtr[0], &cookieSize);
            return py::bytes(cookiePtr);
        }, py::arg("num_channels"), "Get the magic cookie for a specific number of channels.")
        .def("encode", [](ALACEncoder *encoder, AudioFormatDescription &inputFormat, AudioFormatDescription &outputFormat, py::bytes &pcmData)
        {
            py::buffer_info info(py::buffer(pcmData).request());
            // The input data
            unsigned char *data = reinterpret_cast<unsigned char *>(info.ptr);
            // Size of the input data in bytes
            int32_t size = static_cast<int32_t>(info.shape[0] * info.itemsize);
            std::string outBuf(size, 0);
            // Encode the data. It's safe to cast, since char and unsigned char both use one byte.
            encoder->Encode(inputFormat, outputFormat, data, (unsigned char *)&outBuf[0], &size);
            return py::bytes(outBuf);
        }, py::arg("in_format"), py::arg("out_format"), py::arg("pcm_data"), "Encode data to ALAC data.")
        .def_property("frame_size", &ALACEncoder::GetFrameSize, &ALACEncoder::SetFrameSize);

    py::class_<ALACDecoder>(m, "ALACDecoder")
        .def(py::init([](py::bytes &magic_cookie)
        {
            ALACDecoder *decoder = new ALACDecoder();
            py::buffer_info info(py::buffer(magic_cookie).request());
            uint8_t *data = reinterpret_cast<uint8_t *>(info.ptr);
            uint32_t size = static_cast<uint32_t>(info.shape[0] * info.itemsize);
            decoder->Init(data, size);
            return decoder;
        }),  py::arg("magic_cookie"))
        .def("decode", [](ALACDecoder *decoder, AudioFormatDescription &inputFormat, AudioFormatDescription &outputFormat, py::bytes &alacData)
        {
            // input data
            py::buffer_info info(py::buffer(alacData).request());
            uint8_t *data = reinterpret_cast<uint8_t *>(info.ptr);
            uint32_t size = static_cast<uint32_t>(info.shape[0] * info.itemsize);

            BitBuffer inputBuffer;
            BitBufferInit(&inputBuffer, data, size);


            // output buffer
            uint32_t numFrames = 0;
            std::string outBuf(size - kALACMaxEscapeHeaderBytes, 0);

            decoder->Decode(&inputBuffer, (uint8_t *)&outBuf[0], inputFormat.mFramesPerPacket,
                            inputFormat.mChannelsPerFrame, &numFrames);
            return py::bytes(outBuf);
        }, py::arg("in_format"), py::arg("out_format"), py::arg("alac_data"), "Decode data to ALAC data.")
        .def_readwrite("config", &ALACDecoder::mConfig);

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
