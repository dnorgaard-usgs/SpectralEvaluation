#include <SpectralEvaluation/File/TXTFile.h>
#include <SpectralEvaluation/Spectra/Spectrum.h>

namespace SpectrumIO
{

    /** Reads a spectrum from a TXT-file */
    bool CTXTFile::ReadSpectrum(CSpectrum &spec, const std::string &fileName) {
        double col1, col2;

        // Open the file
        FILE *f = fopen(fileName.c_str(), "r");
        if (f == NULL)
            return false;

        // Clear all the information in the spectrum
        spec.Clear();

        // Simply read the spectrum, one pixel at a time
        int length = 0;
        while (length < MAX_SPECTRUM_LENGTH) {
            int nCols = fscanf(f, "%lf %lf", &col1, &col2);
            if (nCols == 0)
                break;
            else if (nCols == 1)
                spec.m_data[length] = col1;
            else if (nCols == 2)
                spec.m_data[length] = col2;

            ++length;
        }
        spec.m_length = length;

        // close the file before we return
        fclose(f);
        return true;
    }

    /** Writes a spectrum to a TXT-file */
    bool CTXTFile::WriteSpectrum(const CSpectrum *spec, const std::string &fileName) {
        if (spec == NULL)
            return false;
        return WriteSpectrum(*spec, fileName);
    }

    /** Writes a spectrum to a TXT-file */
    bool CTXTFile::WriteSpectrum(const CSpectrum &spec, const std::string &fileName) {
        // Open the file
        FILE *f = fopen(fileName.c_str(), "w");
        if (NULL == f)
            return false;

        // Write the spectrum, one data-point at a time
        for (int k = 0; k < spec.m_length; ++k) {
            if (0 > fprintf(f, "%lf\r\n", spec.m_data[k])) {
                // an error has occured, try to close the file
                fclose(f);
                return false;
            }
        }

        fclose(f); // <-- close the file before we return
        return true;
    }

}
