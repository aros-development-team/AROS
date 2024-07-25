# AROS Git Repository [![Codacy Badge](https://app.codacy.com/project/badge/Grade/8dd5a86f87064c14ba75f291c045e788)](https://www.codacy.com/gh/aros-development-team/AROS/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=aros-development-team/AROS&amp;utm_campaign=Badge_Grade)

> This is the main repository for active development of the AROS Operating System.
> The repository contains the main Operating System components, SDK and Build System.


## Nightly Test Builds


* All builds are scheduled to run at 00:00 UTC.
* The builds are made using the scripts/azure-pipelines.yml file. Further details can be found in that file.
* The main AROS target and distfiles are built for each arch.
* The builds are downloadable via http://www.aros.org/nightly1.php [![Download AROS Research Operating System](https://img.shields.io/sourceforge/dt/aros.svg)](https://sourceforge.net/projects/aros/files/nightly2/).

<table>
  <tr>
    <td style="text-align:center">BUILD Arch</td>
    <td colspan=4 style="text-align:center">Status</td>
  </tr>
  <tr>
    <td rowspan=2 style="text-align:center">Toolchain</td>
    <td colspan="4" style="text-align:center">GNU</td>
    <td style="text-align:center">LLVM</td>
  </tr>
  <tr>
    <td style="text-align:center">8.3.0</td>
    <td style="text-align:center">9.1.0</td>
    <td style="text-align:center">10.2.0</td>
    <td style="text-align:center">13.1.0</td>
    <td style="text-align:center">10.0</td>
  </tr>
  <tr>
    <td>amiga-m68k</td>
    <td style="text-align:center">
      <a href="https://dev.azure.com/aros-development-team/AROS/_build/latest?definitionId=14&branchName=master"><img alt="Build Status" src="https://dev.azure.com/aros-development-team/AROS/_apis/build/status/aros-development-team.AROS-amiga-m68k?branchName=master"></a>
    </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
  </tr>
  <tr>
    <td>pc-i386</td>
    <td style="text-align:center">
      <a href="https://dev.azure.com/aros-development-team/AROS/_build/latest?definitionId=16&branchName=master"><img alt="Build Status" src="https://dev.azure.com/aros-development-team/AROS/_apis/build/status/aros-development-team.AROS-pc-i386?branchName=master"></a>
    </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
  </tr>
  <tr>
    <td>pc-x86_64</td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center">
      <a href="https://dev.azure.com/aros-development-team/AROS/_build/latest?definitionId=17&branchName=master"><img alt="Build Status" src="https://dev.azure.com/aros-development-team/AROS/_apis/build/status/aros-development-team.AROS-pc-x86_64?branchName=master"></a>
    </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
  </tr>
  <tr>
    <td>pc-x86_64-smp</td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center">
      <a href="https://dev.azure.com/aros-development-team/AROS/_build/latest?definitionId=15&branchName=master"><img alt="Build Status" src="https://dev.azure.com/aros-development-team/AROS/_apis/build/status/aros-development-team.AROS-pc-x86_64-smp?branchName=master"></a>
    </td>
    <td style="text-align:center"> --- </td>
  </tr>
  <tr>
    <td>raspi-armhf</td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center">
      <a href="https://dev.azure.com/aros-development-team/AROS/_build/latest?definitionId=19&branchName=master"><img alt="Build Status" src="https://dev.azure.com/aros-development-team/AROS/_apis/build/status/aros-development-team.AROS-raspi-armhf?branchName=master"></a>
    </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
  </tr>
  <tr>
    <td>sam440-ppc</td>
    <td style="text-align:center">
      <a href="https://dev.azure.com/aros-development-team/AROS/_build/latest?definitionId=20&branchName=master"><img alt="Build Status" src="https://dev.azure.com/aros-development-team/AROS/_apis/build/status/aros-development-team.AROS-sam440-ppc?branchName=master"></a>
    </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
  </tr>
  <tr>
    <td>linux-i386</td>
    <td style="text-align:center">
      <a href="https://dev.azure.com/aros-development-team/AROS/_build/latest?definitionId=21&branchName=master"><img alt="Build Status" src="https://dev.azure.com/aros-development-team/AROS/_apis/build/status/aros-development-team.AROS-linux-i386?branchName=master"></a>
    </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
  </tr>
  <tr>
    <td>linux-x86_64</td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center">
      <a href="https://dev.azure.com/aros-development-team/AROS/_build/latest?definitionId=18&branchName=master"><img alt="Build Status" src="https://dev.azure.com/aros-development-team/AROS/_apis/build/status/aros-development-team.AROS-linux-x86_64-gnu?branchName=master"></a>
    </td>
    <td style="text-align:center">
      <a href="https://dev.azure.com/aros-development-team/AROS/_build/latest?definitionId=26&branchName=master"><img alt="Build Status" src="https://dev.azure.com/aros-development-team/AROS/_apis/build/status/aros-development-team.AROS-linux-x86_64-llvm?branchName=master"></a>
    </td>
  </tr>
  <tr>
    <td>linux-arm</td>
    <td style="text-align:center">
      <a href="https://dev.azure.com/aros-development-team/AROS/_build/latest?definitionId=29&branchName=master"><img alt="Build Status" src="https://dev.azure.com/aros-development-team/AROS/_apis/build/status/aros-development-team.AROS-linux-arm?branchName=master"></a>
    </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
  </tr>
  <tr>
    <td>linux-armhf</td>
    <td style="text-align:center">
      <a href="https://dev.azure.com/aros-development-team/AROS/_build/latest?definitionId=28&branchName=master"><img alt="Build Status" src="https://dev.azure.com/aros-development-team/AROS/_apis/build/status/aros-development-team.AROS-linux-armhf?branchName=master"></a>
    </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
  </tr>
  <tr>
    <td>darwin-i386</td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center">
      <a href="https://dev.azure.com/aros-development-team/AROS/_build/latest?definitionId=24&branchName=master"><img alt="Build Status" src="https://dev.azure.com/aros-development-team/AROS/_apis/build/status/aros-development-team.AROS-darwin-i386?branchName=master"></a>
    </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
  </tr>
  <tr>
    <td>darwin-x86_64</td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center">
      <a href="https://dev.azure.com/aros-development-team/AROS/_build/latest?definitionId=22&branchName=master"><img alt="Build Status" src="https://dev.azure.com/aros-development-team/AROS/_apis/build/status/aros-development-team.AROS-darwin-x86_64?branchName=master"></a>
    </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
  </tr>
  <tr>
    <td>darwin-ppc</td>
    <td style="text-align:center">
      <a href="https://dev.azure.com/aros-development-team/AROS/_build/latest?definitionId=25&branchName=master"><img alt="Build Status" src="https://dev.azure.com/aros-development-team/AROS/_apis/build/status/aros-development-team.AROS-darwin-ppc?branchName=master"></a>
    </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
  </tr>
  <tr>
    <td>mingw32-i386</td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center">
      <a href="https://dev.azure.com/aros-development-team/AROS/_build/latest?definitionId=23&branchName=master"><img alt="Build Status" src="https://dev.azure.com/aros-development-team/AROS/_apis/build/status/aros-development-team.AROS-mingw32-i386?branchName=master"></a>
    </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
  </tr>
  <tr>
    <td>mingw32-x86_64</td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center">
      <a href="https://dev.azure.com/aros-development-team/AROS/_build/latest?definitionId=30&branchName=master"><img alt="Build Status" src="https://dev.azure.com/aros-development-team/AROS/_apis/build/status/aros-development-team.AROS-mingw32-x86_64?branchName=master"></a>
    </td>
    <td style="text-align:center"> --- </td>
    <td style="text-align:center"> --- </td>
  </tr>
</table>

## Contributing

Please see the [CONTRIBUTING.md](CONTRIBUTING.md) file for details on joining the GitHub organization, and guidelines on contributing to the AROS project.

## License

This project is licensed under the APL License - see the [LICENSE](LICENSE) file for details

## Acknowledgments

AROS contains parts built upon external components - see the [ACKNOWLEDGEMENTS](ACKNOWLEDGEMENTS) file for details

