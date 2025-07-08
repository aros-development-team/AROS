# Sample colors as objects with to_hex() method for simplicity (replace with your actual color code logic)
class Color:
    def __init__(self, hexcode):
        self.hexcode = hexcode
    def to_hex(self):
        return self.hexcode

colorBG = Color("#91ABC6")        # Group header background color
colorFileDL = Color("#EEEEEE")    # Download available cell background
colorFileNA = Color("#CCCCCC")    # Download not available cell background
colorGhosted = Color("#777777")   # Text color for 'not available'

# Sample input data structure for demonstration purposes:
# groups_packages_dates = { group_name: { package_name: [date1, date2, ...], ... }, ... }
# dates = sorted list of all dates found across packages (used as table columns)
groups_packages_dates = {
    "Developer_Tools": {
        "Linux-gnu-amiga-m68k-toolchain": ["2025-07-08"],
        "Linux-gnu-linux-armhf-toolchain": ["2025-07-08"],
        "Linux-gnu-raspi-armhf-toolchain": ["2025-07-08"],
    },
    "Sources": {
        "contrib-source": ["2025-07-08"],
        "source": ["2025-07-08"],
    },
    "EmptyGroup": {
        "empty-package": []
    }
}

# Collect all unique dates sorted (you probably do this earlier in your real script)
dates = sorted({
    date
    for group in groups_packages_dates.values()
    for package_dates in group.values()
    for date in package_dates
})

html = []

# Header & styles omitted for brevity - insert your actual header here if needed

html.append('<h2>AROS nightly2 Downloads</h2>')
html.append('<table class="layout nightly" border="0" cellpadding="2" cellspacing="1" bgcolor="#000000">')
html.append('  <tr class="layout nightly" bgcolor="#91ABC6">')
html.append('    <td class="layout invis" width="35%" rowspan="2"></td>')
html.append('    <td class="layout padded" align="center" colspan="{}"><b>Date</b></td>'.format(len(dates)))
html.append('  </tr>')
html.append('  <tr class="layout nightly" bgcolor="#91ABC6">')
for date in dates:
    html.append(f'<td class="layout nightly" align="center" bgcolor="#EEEEEE"><b>{date}</b></td>')
html.append('  </tr>')

# Main table content
for group in sorted(groups_packages_dates.keys()):
    # Check if this group has any packages with at least one date
    has_content = False
    for package, pkg_dates in groups_packages_dates[group].items():
        if pkg_dates:
            has_content = True
            break
    if not has_content:
        # Skip this group entirely
        continue

    # Print group header row spanning all columns (package + dates)
    html.append(f'<tr class="layout nightly" bgcolor="{colorBG.to_hex()}">')
    html.append(f'<td class="layout padded" align="left" colspan="{1 + len(dates)}"><b>{group}</b></td>')
    html.append('</tr>')

    # Print packages with downloads
    for package in sorted(groups_packages_dates[group].keys()):
        pkg_dates = groups_packages_dates[group][package]
        if not pkg_dates:
            # Skip packages with no downloads at all
            continue
        html.append(f'<tr class="layout nightly" valign="middle" bgcolor="{colorFileDL.to_hex()}">')
        html.append(f'<td class="layout padded" valign="top"><p><i>{package}</i></p><p><font size="-1">No description</font></p></td>')
        for date in dates:
            if date in pkg_dates:
                # Construct your actual download URL here:
                # Example URL structure used in your sample:
                # https://sourceforge.net/projects/arosdev/files/nightly2/{date}/{group}/{filename}/download
                filename = f"AROS-{date.replace('-', '')}-{package}.tar.bz2"
                url = f"https://sourceforge.net/projects/arosdev/files/nightly2/{date}/{group}/{filename}/download"
                html.append(f'<td class="layout nightly" align="center" bgcolor="{colorFileDL.to_hex()}"><a href="{url}" target="_blank">Download</a></td>')
            else:
                html.append(f'<td class="layout nightly" align="center" bgcolor="{colorFileNA.to_hex()}"><font color="{colorGhosted.to_hex()}">not available</font></td>')
        html.append('</tr>')

html.append('</table>')

# Join HTML lines
output_html = "\n".join(html)

# Print or save output as needed
print(output_html)
