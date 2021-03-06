#pragma once

#include <string>

class CSpectrum;

namespace SpectrumIO {

    /** <b>CTXTFile</b> is a simple class for reading/writing spectra
            from/to .txt/.xs - files */
    class CTXTFile
    {
    public:

        /** Reads a spectrum from a TXT-file. @return true if the reading succeeds. */
        static bool ReadSpectrum(CSpectrum &spec, const std::string &fileName);

        /** Writes a spectrum to a TXT-file. @return true if the writing succeeds. */
        static bool WriteSpectrum(const CSpectrum &spec, const std::string &fileName);

        /** Writes a spectrum to a TXT-file. @return true if the writing succeeds. */
        static bool WriteSpectrum(const CSpectrum *spec, const std::string &fileName);

    };
}