"""
Root module of `symbolizer` Python package.
"""

import enum
import pathlib
from typing import Optional, Union

from ._symbolizer import (  # type: ignore
    version,
    target,
    TraceStyle_t as TraceStyle,
    DbgEng_t as _DbgEng_t,
)

# class TraceStyle(enum.IntEnum):
#     Modoff = _TraceStyle_t.Modoff
#     FullSymbol = _TraceStyle_t.FullSymbol


class DebugEngine:
    def __init__(
        self,
        dump_path: Union[str, pathlib.Path],
        symbol_path: Optional[Union[str, pathlib.Path]] = None,
    ):
        if isinstance(dump_path, str):
            dump_path = pathlib.Path(dump_path)

        if not isinstance(dump_path, pathlib.Path):
            raise TypeError

        if not dump_path.exists():
            raise ValueError

        if symbol_path is not None:
            if isinstance(symbol_path, str):
                symbol_path = pathlib.Path(symbol_path)

            if not isinstance(symbol_path, pathlib.Path):
                raise TypeError

            if not symbol_path.exists():
                raise ValueError

        self.__dbgeng = _DbgEng_t()
        if not self.__dbgeng.Init(dump_path, symbol_path):
            raise RuntimeError(f"Failed to initialize {dump_path}")

        self.__dump_path = dump_path
        self.__style = TraceStyle.FullSymbol
        self.__symbol_path = symbol_path

    @property
    def dump_path(self) -> pathlib.Path:
        return self.__dump_path

    @property
    def symbol_path(self) -> Optional[pathlib.Path]:
        return self.__symbol_path

    @property
    def style(self) -> TraceStyle:
        return self.__style

    @style.setter
    def style(self, value: TraceStyle) -> None:
        self.__style = value

    def symbolize(
        self, address: int, style: Optional[TraceStyle] = None
    ) -> str:
        style = style or self.__style
        res = self.__dbgeng.Symbolize(address, style)
        if not res:
            raise ValueError(f"Failed to symbolize address {address:#x}")
        return res

    def resolve(self, symbol_name: str) -> int:
        res = self.__dbgeng.Resolve(symbol_name)
        if not res:
            raise ValueError(f"Failed to resolve symbol '{symbol_name}'")
        return res
