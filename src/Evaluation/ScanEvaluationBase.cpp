#include <SpectralEvaluation/Evaluation/ScanEvaluationBase.h>
#include <SpectralEvaluation/Evaluation/FitWindow.h>
#include <SpectralEvaluation/Evaluation/EvaluationBase.h>
#include <SpectralEvaluation/Spectra/Spectrum.h>
#include <SpectralEvaluation/Spectra/SpectrometerModel.h>
#include <SpectralEvaluation/File/ScanFileHandler.h>
#include <SpectralEvaluation/File/STDFile.h>
#include <SpectralEvaluation/File/TXTFile.h>
#include <SpectralEvaluation/Utils.h>
#include <SpectralEvaluation/Configuration/DarkSettings.h>
#include <SpectralEvaluation/Configuration/SkySettings.h>

#include <sstream>

namespace Evaluation
{
    ScanEvaluationBase::ScanEvaluationBase()
    {

    }

    ScanEvaluationBase::~ScanEvaluationBase()
    {

    }

    bool ScanEvaluationBase::ReadSpectrumFromFile(const std::string& fullFilename, CSpectrum& spec)
    {
        if (fullFilename.size() < 3)
        {
            return false;
        }
        if (SpectrumIO::CSTDFile::ReadSpectrum(spec, fullFilename))
        {
            return true;
        }
        if (SpectrumIO::CTXTFile::ReadSpectrum(spec, fullFilename))
        {
            return true;
        }
        return false;
    }

    bool ScanEvaluationBase::GetOffsetSpectrum(FileHandler::CScanFileHandler &scan, const Configuration::CDarkSettings& darkSettings, CSpectrum &offsetSpectrum)
    {
        if (darkSettings.m_offsetOption == Configuration::DARK_MODEL_OPTION::USER_SUPPLIED)
        {
            return ReadSpectrumFromFile(darkSettings.m_offsetSpec, offsetSpectrum);
        }
        else
        {
            return 0 != scan.GetOffset(offsetSpectrum);
        }
    }

    bool ScanEvaluationBase::GetDarkCurrentSpectrum(FileHandler::CScanFileHandler &scan, const Configuration::CDarkSettings& darkSettings, CSpectrum &darkCurrent, bool& needsOffsetCorrection)
    {
        needsOffsetCorrection = true;
        if (darkSettings.m_darkCurrentOption == Configuration::DARK_MODEL_OPTION::USER_SUPPLIED)
        {
            if (ReadSpectrumFromFile(darkSettings.m_darkCurrentSpec, darkCurrent))
            {
                needsOffsetCorrection = false;
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return 0 != scan.GetDarkCurrent(darkCurrent);
        }
    }

    bool ScanEvaluationBase::ModelDarkSpectrum(FileHandler::CScanFileHandler &scan, const Configuration::CDarkSettings& darkSettings, const CSpectrum &spec, CSpectrum &dark)
    {
        bool offsetCorrectDC = true;

        CSpectrum offset;
        if (!GetOffsetSpectrum(scan, darkSettings, offset))
        {
            m_lastErrorMessage = "Could not retrieve an offset spectrum from the scan.";
            return false;
        }

        CSpectrum darkCurrent;
        if (!GetDarkCurrentSpectrum(scan, darkSettings, darkCurrent, offsetCorrectDC))
        {
            m_lastErrorMessage = "Could not retrieve a dark-current spectrum from the scan.";
            return false;
        }

        if (offset.m_length == darkCurrent.m_length && offset.m_length > 0)
        {
            // 3c-1 Scale the offset spectrum to the measured
            offset.Mult(spec.NumSpectra() / (double)offset.NumSpectra());
            offset.m_info.m_numSpec = spec.NumSpectra();

            // 3c-2 Remove offset from the dark-current spectrum
            if (offsetCorrectDC)
            {
                CSpectrum offset_dc = offset;
                offset_dc.Mult(darkCurrent.NumSpectra() / (double)offset_dc.NumSpectra());
                darkCurrent.Sub(offset_dc);
            }

            // 3c-3 Scale the dark-current spectrum to the measured
            darkCurrent.Mult((spec.NumSpectra() * spec.ExposureTime()) / (double)(darkCurrent.NumSpectra() * darkCurrent.ExposureTime()));
            darkCurrent.m_info.m_numSpec = spec.NumSpectra();

            // 3d. Make the dark-spectrum
            dark.Clear();
            dark.m_length = offset.m_length;
            dark.m_info.m_interlaceStep = offset.m_info.m_interlaceStep;
            dark.m_info.m_channel = offset.m_info.m_channel;
            dark.Add(offset);
            dark.Add(darkCurrent);

            // If the dark-spectrum is read out in an interlaced way then interpolate it back to it's original state
            if (dark.m_info.m_interlaceStep > 1)
            {
                dark.InterpolateSpectrum();
            }

            return true;
        }
        else
        {
            m_lastErrorMessage = "Invalid offset/dark-current spectra the lengths does not agree.";
            return false;
        }
    }

    bool ScanEvaluationBase::GetDark(FileHandler::CScanFileHandler& scan, const CSpectrum &spec, CSpectrum &dark, const Configuration::CDarkSettings* darkSettings)
    {
        // 1. The user wants to take the dark spectrum directly from the measurement
        //      as the second spectrum in the scan.
        if (darkSettings == nullptr || darkSettings->m_darkSpecOption == Configuration::DARK_SPEC_OPTION::MEASURED_IN_SCAN)
        {
            if (0 != scan.GetDark(dark))
            {
                m_lastErrorMessage = "Could not read dark-spectrum from scan " + scan.GetFileName();
                return false;
            }
            return true;
        }
        else if (darkSettings->m_darkSpecOption == Configuration::DARK_SPEC_OPTION::MODEL_WHEN_NOT_MEASURED_IN_SCAN)
        {
            if (0 != scan.GetDark(dark))
            {
                m_lastErrorMessage = "Could not read dark-spectrum from scan " + scan.GetFileName() + " proceeds to modelling the dark from offset + dark-current";
            }

            // if there is no dark spectrum but one offset and one dark-current spectrum,
            //   then read those instead and model the dark spectrum
            if (dark.m_length == 0)
            {
                if (ModelDarkSpectrum(scan, *darkSettings, spec, dark))
                {
                    m_lastErrorMessage = "Warning: Incorrect settings: check settings for dark current correction";
                    return false;
                }
                else
                {
                    m_lastErrorMessage = "WARNING: NO DARK SPECTRUM FOUND IN SCAN. INCORRECT DARK CURRENT CORRECTION";
                    return false;
                }
            }

            // If the dark-spectrum is read out in an interlaced way then interpolate it back to it's original state
            if (dark.m_info.m_interlaceStep > 1) {
                dark.InterpolateSpectrum();
            }

            // Check so that the exposure-time of the dark-spectrum is same as the
            //   exposure time of the measured spectrum
            if (dark.ExposureTime() != spec.ExposureTime()) {
                m_lastErrorMessage = "WARNING: EXPOSURE-TIME OF DARK-SPECTRUM IS NOT SAME AS FOR MEASURED SPECTRUM. INCORRECT DARK-CORRECTION!! " + scan.GetFileName();
            }

            // Make sure that there are the same number of exposures in the
            //  dark-spectrum as in the measured spectrum
            if (dark.NumSpectra() != spec.NumSpectra()) {
                dark.Mult(spec.NumSpectra() / (double)dark.NumSpectra());
            }

            return true;
        }

        // 3. The user wants to model the dark spectrum
        if (darkSettings->m_darkSpecOption == Configuration::DARK_SPEC_OPTION::MODEL_ALWAYS)
        {
            if (ModelDarkSpectrum(scan, *darkSettings, spec, dark))
            {
                return true;
            }
            else
            {
                m_lastErrorMessage = "Warning: Incorrect settings: check settings for dark current correction";
                return false;
            }
        }

        // 4. The user has his own favourite dark-spectrum that he wants to use
        if (darkSettings->m_darkSpecOption == Configuration::DARK_SPEC_OPTION::USER_SUPPLIED)
        {
            if (!ReadSpectrumFromFile(darkSettings->m_offsetSpec, dark))
            {
                m_lastErrorMessage = "Error: Failed to read the dark-spectrum: " + darkSettings->m_offsetSpec + " cannot evaluate scan";
                return false;
            }

            // If the dark-spectrum is read out in an interlaced way then interpolate it back to it's original state
            if (dark.m_info.m_interlaceStep > 1)
            {
                dark.InterpolateSpectrum();
            }

            return true;
        }

        // something is not implemented
        m_lastErrorMessage = "Error: Failed to get dark spectrum for scan, not implemented option found.";
        return false;
    }

    bool ScanEvaluationBase::GetSky(FileHandler::CScanFileHandler& scan, const Configuration::CSkySettings& settings, CSpectrum &sky)
    {
        // If the sky spectrum is the first spectrum in the scan
        if (settings.skyOption == Configuration::SKY_OPTION::MEASURED_IN_SCAN)
        {
            if (0 != scan.GetSky(sky))
            {
                m_lastErrorMessage = "Could not get sky spectrum from scan, no spectrum is labelled as sky.";
                return false;
            }

            if (sky.m_info.m_interlaceStep > 1)
            {
                sky.InterpolateSpectrum();
            }

            return true;
        }

        // If the sky spectrum is the average of all credible spectra
        if (settings.skyOption == Configuration::SKY_OPTION::AVERAGE_OF_GOOD_SPECTRA_IN_SCAN)
        {
            if (!GetSkySpectrumFromAverageOfGoodSpectra(scan, sky))
            {
                m_lastErrorMessage = "Failed to get sky spectrum from scan, no spectra are good enough.";
            }
            return true;
        }

        // If the user wants to use another spectrum than 'sky' as reference-spectrum...
        if (settings.skyOption == Configuration::SKY_OPTION::SPECTRUM_INDEX_IN_SCAN)
        {
            if (0 == scan.GetSpectrum(sky, settings.indexInScan))
            {
                m_lastErrorMessage = "Failed to get sky spectrum from index in scan, could not find given spectrum.";
                return false;
            }

            if (sky.m_info.m_interlaceStep > 1)
            {
                sky.InterpolateSpectrum();
            }

            return true;
        }

        // If the user has supplied a special sky-spectrum to use
        if (settings.skyOption == Configuration::SKY_OPTION::USER_SUPPLIED)
        {
            return GetSkySpectrumFromFile(settings.skySpectrumFile, sky);
        }

        return false;
    }

    bool ScanEvaluationBase::GetSkySpectrumFromAverageOfGoodSpectra(FileHandler::CScanFileHandler& scan, CSpectrum &sky) const
    {
        const int interlaceSteps = scan.GetInterlaceSteps();
        const int startChannel = scan.GetStartChannel();
        const int fitLow = m_fitLow / interlaceSteps - startChannel;
        const int fitHigh = m_fitHigh / interlaceSteps - startChannel;
        int nofSpectraAveraged = 0;

        CSpectrum tmp;
        scan.GetSky(tmp);
        scan.ResetCounter();

        // Get the maximum intensity of this spectrometer model (with a little bit of margin)
        const double spectrometerDynamicRange = (CSpectrometerDatabase::GetInstance().GetModel(tmp.m_info.m_specModelName).maximumIntensity - 20);

        double spectrumIntensityInFitRegion = tmp.MaxValue(fitLow, fitHigh);
        if (spectrumIntensityInFitRegion < spectrometerDynamicRange * tmp.NumSpectra() && !tmp.IsDark())
        {
            sky = tmp;
            nofSpectraAveraged = 1;
        }
        else
        {
            sky.Clear();
        }

        while (scan.GetNextSpectrum(tmp))
        {
            spectrumIntensityInFitRegion = tmp.MaxValue(fitLow, fitHigh);
            if (spectrumIntensityInFitRegion < spectrometerDynamicRange * tmp.NumSpectra() && !tmp.IsDark())
            {
                sky.Add(tmp);
                ++nofSpectraAveraged;
            }
        }
        scan.ResetCounter();

        if (sky.m_info.m_interlaceStep > 1)
        {
            sky.InterpolateSpectrum();
        }

        return (nofSpectraAveraged > 0);
    }

    bool ScanEvaluationBase::GetSkySpectrumFromFile(const std::string& filename, CSpectrum& sky)
    {
        if (filename.size() < 5)
        {
            m_lastErrorMessage = "Cannot get the sky-spectrum, the filename was empty";
            return false;
        }
        const std::string extension = Right(filename, 4);

        if (EqualsIgnoringCase(extension, ".pak"))
        {
            SpectrumIO::CSpectrumIO reader;
            return reader.ReadSpectrum(filename, 0, sky);
        }
        
        if (EqualsIgnoringCase(extension, ".std"))
        {
            return SpectrumIO::CSTDFile::ReadSpectrum(sky, filename);
        }

        // If we don't recognize the sky-spectrum format
        m_lastErrorMessage = "Unknown format for sky spectrum. Please use .pak or .std";
        return false;
    }


    int ScanEvaluationBase::GetIndexOfSpectrumWithBestIntensity(const CFitWindow &fitWindow, FileHandler::CScanFileHandler& scan)
    {
        const int INDEX_OF_SKYSPECTRUM = -1;
        const int NO_SPECTRUM_INDEX = -2;
        CSpectrum sky;

        // Find the spectrum for which we should determine shift & squeeze
        //      This spectrum should have high enough intensity in the fit-region
        //      without being saturated.
        int indexOfMostSuitableSpectrum = NO_SPECTRUM_INDEX;
        scan.GetSky(sky);
        double fitSaturation = -1.0;
        double bestSaturation = -1.0;
        double fitIntensity = sky.MaxValue(fitWindow.fitLow, fitWindow.fitHigh);
        double maxInt = CSpectrometerDatabase::GetInstance().GetModel(sky.m_info.m_specModelName).maximumIntensity;
        
        if (sky.NumSpectra() > 0)
        {
            fitSaturation = fitIntensity / (sky.NumSpectra() * maxInt);
        }
        else
        {
            fitSaturation = fitIntensity / (maxInt * sky.NumSpectra());
        }
        
        if (fitSaturation < 0.9 && fitSaturation > 0.1)
        {
            indexOfMostSuitableSpectrum = INDEX_OF_SKYSPECTRUM;
            bestSaturation = fitSaturation;
        }

        scan.ResetCounter(); // start from the beginning

        CSpectrum spectrum;
        int curIndex = 0;
        while (scan.GetNextSpectrum(spectrum))
        {
            fitIntensity = spectrum.MaxValue(fitWindow.fitLow, fitWindow.fitHigh);
            maxInt = CSpectrometerDatabase::GetInstance().GetModel(spectrum.m_info.m_specModelName).maximumIntensity;

            // Get the saturation-ratio for this spectrum
            if (spectrum.NumSpectra() > 0)
            {
                fitSaturation = fitIntensity / (spectrum.NumSpectra() * maxInt);
            }
            else
            {
                fitSaturation = fitIntensity / (maxInt * spectrum.NumSpectra());
            }

            // Check if this spectrum is good...
            if (fitSaturation < 0.9 && fitSaturation > 0.1)
            {
                if (fitSaturation > bestSaturation)
                {
                    indexOfMostSuitableSpectrum = curIndex;
                    bestSaturation = fitSaturation;
                }
            }

            // Go to the next spectrum
            ++curIndex;
        }

        return indexOfMostSuitableSpectrum;
    }


    CEvaluationBase* ScanEvaluationBase::FindOptimumShiftAndSqueezeFromFraunhoferReference(
        const CFitWindow &fitWindow,
        const Configuration::CDarkSettings& darkSettings,
        const Configuration::CSkySettings& skySettings,
        FileHandler::CScanFileHandler& scan)
    {
        // Check that the Fraunhofer reference has been read in
        if (fitWindow.fraunhoferRef.m_data == nullptr || fitWindow.fraunhoferRef.m_data->m_crossSection.size() == 0)
        {
            return nullptr;
        }

        const int INDEX_OF_SKYSPECTRUM = -1;
        const int NO_SPECTRUM_INDEX = -2;

        // 1. Find the spectrum for which we should determine shift & squeeze
        //      This spectrum should have high enough intensity in the fit-region
        //      without being saturated.
        const int indexOfMostSuitableSpectrum = GetIndexOfSpectrumWithBestIntensity(fitWindow, scan);

        // 2. Get the spectrum we should evaluate...
        CSpectrum spectrum;
        if (indexOfMostSuitableSpectrum == NO_SPECTRUM_INDEX)
        {
            m_lastErrorMessage = "  Could not find any suitable spectrum to determine shift from.";
            return nullptr; // we could not find any good spectrum to use...
        }
        else if (indexOfMostSuitableSpectrum == INDEX_OF_SKYSPECTRUM)
        {
            scan.GetSky(spectrum);
            m_lastErrorMessage = "Determining shift and squeeze from sky-spectrum";
        }
        else
        {
            scan.GetSpectrum(spectrum, indexOfMostSuitableSpectrum);
            std::stringstream msg;
            msg << "Determining shift and squeeze from spectrum " << indexOfMostSuitableSpectrum;
            m_lastErrorMessage = msg.str();
        }


        if (spectrum.NumSpectra() > 0 && !m_averagedSpectra)
        {
            spectrum.Div(spectrum.NumSpectra());
        }

        CSpectrum dark;
        if (!GetDark(scan, spectrum, dark, &darkSettings))
        {
            m_lastErrorMessage = "Failed to get dark spectrum, determination of shift-and-squeeze from Fraunhofer lines failed.";
            return nullptr; // fail
        }
        if (dark.NumSpectra() > 0 && !m_averagedSpectra)
        {
            dark.Div(dark.NumSpectra());
        }
        spectrum.Sub(dark);


        CSpectrum sky;
        if (!GetSky(scan, skySettings, sky))
        {
            m_lastErrorMessage = "Failed to get sky spectrum, determination of shift-and-squeeze from Fraunhofer lines failed.";
            return nullptr; // fail
        }
        if (sky.NumSpectra() > 0 && !m_averagedSpectra)
        {
            sky.Div(sky.NumSpectra());
        }

        // 3. Do the evaluation.
        CFitWindow copyOfFitWindow = fitWindow;
        CEvaluationBase shiftEvaluator(copyOfFitWindow);
        shiftEvaluator.SetSkySpectrum(sky);

        double shift, shiftError, squeeze, squeezeError;
        if (shiftEvaluator.EvaluateShift(spectrum, shift, shiftError, squeeze, squeezeError))
        {
            // We failed to make the fit, what shall we do now??
            std::stringstream msg;
            msg << "Failed to determine shift and squeeze in scan " << scan.GetFileName() << ". Will proceed with default parameters";
            m_lastErrorMessage = msg.str();
            return nullptr;
        }

        if (fabs(shiftError) < 1 && fabs(squeezeError) < 0.01)
        {
            CFitWindow improvedFitWindow = fitWindow;

            // The fit is good enough to use the values
            for (int it = 0; it < improvedFitWindow.nRef; ++it)
            {
                improvedFitWindow.ref[it].m_shiftOption     = SHIFT_FIX;
                improvedFitWindow.ref[it].m_squeezeOption   = SHIFT_FIX;
                improvedFitWindow.ref[it].m_shiftValue      = shift;
                improvedFitWindow.ref[it].m_squeezeValue    = squeeze;
            }

            std::stringstream msg;
            msg << "  Shift: " << shift << " +- " << shiftError << "; Squeeze: " << squeeze << " +- " << squeezeError;
            m_lastErrorMessage = msg.str();
            return new CEvaluationBase(improvedFitWindow);
        }

        m_lastErrorMessage = "Fit not good enough. Will proceed with default parameters.";
        return nullptr;
    }

}