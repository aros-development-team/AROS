# VirtIO-GPU manual diagnostic

`virtiogpuprobe` exercises a dedicated secondary, non-VGA modern VirtIO-GPU on x86_64 AROS. It is a diagnostic executable, not a display driver or Gallium backend. It never sets a scanout and does not replace the active graphics driver. It is excluded from automatic quick tests and ROM images.

Build in a configured x86_64 AROS tree:

```sh
make test-virtio-gpu
```

The executable is installed under `Developer/Debug/Tests/hidds/gfx/virtiogpuprobe`. Other architectures are deliberately outside this target's scope; the wire layouts and cache/barrier handling are qualified only on x86_64.

## Hardware and invocation

Keep the existing display adapter and add exactly one dedicated `virtio-gpu-pci` for the 2D control test, or `virtio-gpu-gl-pci` for the VirGL rendering test. For example, the relevant QEMU options are `-vga std -device virtio-gpu-pci`, or `-vga std -device virtio-gpu-gl-pci` plus a host-supported GL display backend. Do not substitute `virtio-vga` or use the tested GPU as the guest's primary display. No other driver may own or initialize the secondary device.

```text
virtiogpuprobe
virtiogpuprobe RUN
virtiogpuprobe RUN REQUIREVIRGL
virtiogpuprobe RUN RENDER
```

Without RUN, only PCI discovery and capability inspection occur. RUN obtains PCI ownership, negotiates modern VirtIO features, queries display information and available capsets, then resets and releases the device. REQUIREVIRGL rejects a device without VirGL and a matching capset. RENDER implies REQUIREVIRGL and creates only offscreen resources.

The render test checks red and green RGBA8 clears, a 4x6 region copy and a nearest 16x16-to-3x2 blit. Each transfer back to guest memory is fenced, and every destination pixel and unchanged border pixel is compared. Source colors are uniform; this does not qualify textured filtering, blending, shader drawing or performance. Run it twice to check reset/resource teardown. No files are written unless the caller redirects output.

Success returns 0. Missing/ambiguous devices, invalid arguments, unsupported capabilities, response/fence errors or pixel mismatches return 20. `RENDER` or `REQUIREVIRGL` without RUN returns 20. On a 2D-only device, RUN should pass and RUN REQUIREVIRGL should return 20. Ctrl-C aborts command polling and follows the same cleanup path.

## Ownership and failure handling

The probe refuses a nonzero VirtIO device status before enabling bus mastering or changing interrupt delivery. It uses PCI-allocated DMA memory, polls bounded split queues and validates response sizes, descriptor IDs and fence completion. Normal cleanup resets the device, restores the PCI command register and releases memory, maps and ownership.

If reset does not complete, the probe disables bus mastering and legacy interrupts, and deliberately retains DMA allocations and PCI ownership until reboot. Its owner string remains allocated after program exit. Do not try to reclaim the device or force its DMA back on in that condition. This is an exceptional quarantine, not a normal allocation lifecycle.

## Verified scope and references

Initial qualification: x86_64 ABIv1, Clang 23.1.0, official nightly 20261031 guest under QEMU TCG. Stock QEMU's 2D device passed control queries and the missing-VirGL negative case. VirGL offscreen operations and reset/retry passed through a separate GL-enabled QEMU build. No ABIv11, non-x86 target, hotplug, multiple-client, scanout or Gallium/application integration qualification is claimed.

The macOS host fork used for qualification needed its Cocoa backend to attach a GL context to the secondary graphical console. An unmodified backend that attaches it only to the default console aborts during VirGL initialization; this host issue is separate from the AROS test. A correctly configured Linux GL backend is also a suitable test host, but was not used for this native AROS qualification.

Protocol: OASIS VirtIO 1.3 section 5.7 (GPU Device), and Mesa 26.2.5 `src/virtio/virtio-gpu/virgl_protocol.h` / `virgl_hw.h`. The local header carries the original Red Hat notice for the Mesa-derived numeric definitions; no Linux driver code is imported. The remaining implementation is contributed under the AROS Public License.
