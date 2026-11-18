#!/usr/bin/env python3
import argparse
from pathlib import Path
from tempfile import TemporaryDirectory
import sys


FRAME_COUNT = 150
TIERS = ((768, 576), (1536, 1152))


def generate(output: Path) -> None:
    import torch

    if sys.byteorder != "little" or torch.__version__.split("+", 1)[0] != "2.12.0":
        raise RuntimeError("Pipeline SEEDED requires little-endian PyTorch 2.12.0")
    torch.set_num_threads(6)
    torch.set_num_interop_threads(12)
    output.mkdir(parents=True, exist_ok=True)
    names = [f"noise_cpu_{width}.f32" for width, _ in TIERS]
    for name in names:
        if (output / name).exists() or (output / name).is_symlink():
            raise FileExistsError(output / name)

    with TemporaryDirectory(dir=output) as staging_path:
        staging = Path(staging_path)
        for (width, height), name in zip(TIERS, names):
            with (staging / name).open("xb") as stream:
                for frame in range(FRAME_COUNT):
                    torch.manual_seed(frame)
                    for divisor in (4, 2):
                        inv_depth = torch.empty(
                            (1, 1, height // divisor, width // divisor),
                            dtype=torch.float32,
                            device="cpu",
                        )
                        noise = torch.randn_like(inv_depth)
                        stream.write(noise.numpy().tobytes(order="C"))
        for name in names:
            (output / name).hardlink_to(staging / name)


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate the two Pipeline SEEDED CPU noise banks.")
    parser.add_argument("output", type=Path)
    generate(parser.parse_args().output)


if __name__ == "__main__":
    main()
