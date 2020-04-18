/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2013 Brian P. Hinz
 * Copyright (C) 2001 Markus G. Kuhn, University of Cambridge
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */
//
// Derived from keysym2ucs.c, originally authored by Markus G, Kuhn
//

package com.tigervnc.rfb;

public class UnicodeToKeysym {

  private static class codepair {
    public codepair(int keysym_, int ucs_) {
      keysym = keysym_;
      ucs = ucs_;
    }
    int keysym;
    int ucs;
  }

  public static codepair[] keysymtab = {
    new codepair(0x01a1, 0x0104), /*                     Aogonek TIN CAPITAL LETTER A WITH OGONEK */
    new codepair(0x01a2, 0x02d8), /*                       breve EVE */
    new codepair(0x01a3, 0x0141), /*                     Lstroke TIN CAPITAL LETTER L WITH STROKE */
    new codepair(0x01a5, 0x013d), /*                      Lcaron TIN CAPITAL LETTER L WITH CARON */
    new codepair(0x01a6, 0x015a), /*                      Sacute TIN CAPITAL LETTER S WITH ACUTE */
    new codepair(0x01a9, 0x0160), /*                      Scaron TIN CAPITAL LETTER S WITH CARON */
    new codepair(0x01aa, 0x015e), /*                    Scedilla TIN CAPITAL LETTER S WITH CEDILLA */
    new codepair(0x01ab, 0x0164), /*                      Tcaron TIN CAPITAL LETTER T WITH CARON */
    new codepair(0x01ac, 0x0179), /*                      Zacute TIN CAPITAL LETTER Z WITH ACUTE */
    new codepair(0x01ae, 0x017d), /*                      Zcaron TIN CAPITAL LETTER Z WITH CARON */
    new codepair(0x01af, 0x017b), /*                   Zabovedot TIN CAPITAL LETTER Z WITH DOT ABOVE */
    new codepair(0x01b1, 0x0105), /*                     aogonek TIN SMALL LETTER A WITH OGONEK */
    new codepair(0x01b2, 0x02db), /*                      ogonek ONEK */
    new codepair(0x01b3, 0x0142), /*                     lstroke TIN SMALL LETTER L WITH STROKE */
    new codepair(0x01b5, 0x013e), /*                      lcaron TIN SMALL LETTER L WITH CARON */
    new codepair(0x01b6, 0x015b), /*                      sacute TIN SMALL LETTER S WITH ACUTE */
    new codepair(0x01b7, 0x02c7), /*                       caron RON */
    new codepair(0x01b9, 0x0161), /*                      scaron TIN SMALL LETTER S WITH CARON */
    new codepair(0x01ba, 0x015f), /*                    scedilla TIN SMALL LETTER S WITH CEDILLA */
    new codepair(0x01bb, 0x0165), /*                      tcaron TIN SMALL LETTER T WITH CARON */
    new codepair(0x01bc, 0x017a), /*                      zacute TIN SMALL LETTER Z WITH ACUTE */
    new codepair(0x01bd, 0x02dd), /*                 doubleacute UBLE ACUTE ACCENT */
    new codepair(0x01be, 0x017e), /*                      zcaron TIN SMALL LETTER Z WITH CARON */
    new codepair(0x01bf, 0x017c), /*                   zabovedot TIN SMALL LETTER Z WITH DOT ABOVE */
    new codepair(0x01c0, 0x0154), /*                      Racute TIN CAPITAL LETTER R WITH ACUTE */
    new codepair(0x01c3, 0x0102), /*                      Abreve TIN CAPITAL LETTER A WITH BREVE */
    new codepair(0x01c5, 0x0139), /*                      Lacute TIN CAPITAL LETTER L WITH ACUTE */
    new codepair(0x01c6, 0x0106), /*                      Cacute TIN CAPITAL LETTER C WITH ACUTE */
    new codepair(0x01c8, 0x010c), /*                      Ccaron TIN CAPITAL LETTER C WITH CARON */
    new codepair(0x01ca, 0x0118), /*                     Eogonek TIN CAPITAL LETTER E WITH OGONEK */
    new codepair(0x01cc, 0x011a), /*                      Ecaron TIN CAPITAL LETTER E WITH CARON */
    new codepair(0x01cf, 0x010e), /*                      Dcaron TIN CAPITAL LETTER D WITH CARON */
    new codepair(0x01d0, 0x0110), /*                     Dstroke TIN CAPITAL LETTER D WITH STROKE */
    new codepair(0x01d1, 0x0143), /*                      Nacute TIN CAPITAL LETTER N WITH ACUTE */
    new codepair(0x01d2, 0x0147), /*                      Ncaron TIN CAPITAL LETTER N WITH CARON */
    new codepair(0x01d5, 0x0150), /*                Odoubleacute TIN CAPITAL LETTER O WITH DOUBLE ACUTE */
    new codepair(0x01d8, 0x0158), /*                      Rcaron TIN CAPITAL LETTER R WITH CARON */
    new codepair(0x01d9, 0x016e), /*                       Uring TIN CAPITAL LETTER U WITH RING ABOVE */
    new codepair(0x01db, 0x0170), /*                Udoubleacute TIN CAPITAL LETTER U WITH DOUBLE ACUTE */
    new codepair(0x01de, 0x0162), /*                    Tcedilla TIN CAPITAL LETTER T WITH CEDILLA */
    new codepair(0x01e0, 0x0155), /*                      racute TIN SMALL LETTER R WITH ACUTE */
    new codepair(0x01e3, 0x0103), /*                      abreve TIN SMALL LETTER A WITH BREVE */
    new codepair(0x01e5, 0x013a), /*                      lacute TIN SMALL LETTER L WITH ACUTE */
    new codepair(0x01e6, 0x0107), /*                      cacute TIN SMALL LETTER C WITH ACUTE */
    new codepair(0x01e8, 0x010d), /*                      ccaron TIN SMALL LETTER C WITH CARON */
    new codepair(0x01ea, 0x0119), /*                     eogonek TIN SMALL LETTER E WITH OGONEK */
    new codepair(0x01ec, 0x011b), /*                      ecaron TIN SMALL LETTER E WITH CARON */
    new codepair(0x01ef, 0x010f), /*                      dcaron TIN SMALL LETTER D WITH CARON */
    new codepair(0x01f0, 0x0111), /*                     dstroke TIN SMALL LETTER D WITH STROKE */
    new codepair(0x01f1, 0x0144), /*                      nacute TIN SMALL LETTER N WITH ACUTE */
    new codepair(0x01f2, 0x0148), /*                      ncaron TIN SMALL LETTER N WITH CARON */
    new codepair(0x01f5, 0x0151), /*                odoubleacute TIN SMALL LETTER O WITH DOUBLE ACUTE */
    new codepair(0x01f8, 0x0159), /*                      rcaron TIN SMALL LETTER R WITH CARON */
    new codepair(0x01f9, 0x016f), /*                       uring TIN SMALL LETTER U WITH RING ABOVE */
    new codepair(0x01fb, 0x0171), /*                udoubleacute TIN SMALL LETTER U WITH DOUBLE ACUTE */
    new codepair(0x01fe, 0x0163), /*                    tcedilla TIN SMALL LETTER T WITH CEDILLA */
    new codepair(0x01ff, 0x02d9), /*                    abovedot T ABOVE */
    new codepair(0x02a1, 0x0126), /*                     Hstroke TIN CAPITAL LETTER H WITH STROKE */
    new codepair(0x02a6, 0x0124), /*                 Hcircumflex TIN CAPITAL LETTER H WITH CIRCUMFLEX */
    new codepair(0x02a9, 0x0130), /*                   Iabovedot TIN CAPITAL LETTER I WITH DOT ABOVE */
    new codepair(0x02ab, 0x011e), /*                      Gbreve TIN CAPITAL LETTER G WITH BREVE */
    new codepair(0x02ac, 0x0134), /*                 Jcircumflex TIN CAPITAL LETTER J WITH CIRCUMFLEX */
    new codepair(0x02b1, 0x0127), /*                     hstroke TIN SMALL LETTER H WITH STROKE */
    new codepair(0x02b6, 0x0125), /*                 hcircumflex TIN SMALL LETTER H WITH CIRCUMFLEX */
    new codepair(0x02b9, 0x0131), /*                    idotless TIN SMALL LETTER DOTLESS I */
    new codepair(0x02bb, 0x011f), /*                      gbreve TIN SMALL LETTER G WITH BREVE */
    new codepair(0x02bc, 0x0135), /*                 jcircumflex TIN SMALL LETTER J WITH CIRCUMFLEX */
    new codepair(0x02c5, 0x010a), /*                   Cabovedot TIN CAPITAL LETTER C WITH DOT ABOVE */
    new codepair(0x02c6, 0x0108), /*                 Ccircumflex TIN CAPITAL LETTER C WITH CIRCUMFLEX */
    new codepair(0x02d5, 0x0120), /*                   Gabovedot TIN CAPITAL LETTER G WITH DOT ABOVE */
    new codepair(0x02d8, 0x011c), /*                 Gcircumflex TIN CAPITAL LETTER G WITH CIRCUMFLEX */
    new codepair(0x02dd, 0x016c), /*                      Ubreve TIN CAPITAL LETTER U WITH BREVE */
    new codepair(0x02de, 0x015c), /*                 Scircumflex TIN CAPITAL LETTER S WITH CIRCUMFLEX */
    new codepair(0x02e5, 0x010b), /*                   cabovedot TIN SMALL LETTER C WITH DOT ABOVE */
    new codepair(0x02e6, 0x0109), /*                 ccircumflex TIN SMALL LETTER C WITH CIRCUMFLEX */
    new codepair(0x02f5, 0x0121), /*                   gabovedot TIN SMALL LETTER G WITH DOT ABOVE */
    new codepair(0x02f8, 0x011d), /*                 gcircumflex TIN SMALL LETTER G WITH CIRCUMFLEX */
    new codepair(0x02fd, 0x016d), /*                      ubreve TIN SMALL LETTER U WITH BREVE */
    new codepair(0x02fe, 0x015d), /*                 scircumflex TIN SMALL LETTER S WITH CIRCUMFLEX */
    new codepair(0x03a2, 0x0138), /*                         kra TIN SMALL LETTER KRA */
    new codepair(0x03a3, 0x0156), /*                    Rcedilla TIN CAPITAL LETTER R WITH CEDILLA */
    new codepair(0x03a5, 0x0128), /*                      Itilde TIN CAPITAL LETTER I WITH TILDE */
    new codepair(0x03a6, 0x013b), /*                    Lcedilla TIN CAPITAL LETTER L WITH CEDILLA */
    new codepair(0x03aa, 0x0112), /*                     Emacron APITAL LETTER E WITH MACRON */
    new codepair(0x03ab, 0x0122), /*                    Gcedilla TIN CAPITAL LETTER G WITH CEDILLA */
    new codepair(0x03ac, 0x0166), /*                      Tslash TIN CAPITAL LETTER T WITH STROKE */
    new codepair(0x03b3, 0x0157), /*                    rcedilla TIN SMALL LETTER R WITH CEDILLA */
    new codepair(0x03b5, 0x0129), /*                      itilde TIN SMALL LETTER I WITH TILDE */
    new codepair(0x03b6, 0x013c), /*                    lcedilla TIN SMALL LETTER L WITH CEDILLA */
    new codepair(0x03ba, 0x0113), /*                     emacron TIN SMALL LETTER E WITH MACRON */
    new codepair(0x03bb, 0x0123), /*                    gcedilla TIN SMALL LETTER G WITH CEDILLA */
    new codepair(0x03bc, 0x0167), /*                      tslash TIN SMALL LETTER T WITH STROKE */
    new codepair(0x03bd, 0x014a), /*                         ENG TIN CAPITAL LETTER ENG */
    new codepair(0x03bf, 0x014b), /*                         eng TIN SMALL LETTER ENG */
    new codepair(0x03c0, 0x0100), /*                     Amacron TIN CAPITAL LETTER A WITH MACRON */
    new codepair(0x03c7, 0x012e), /*                     Iogonek TIN CAPITAL LETTER I WITH OGONEK */
    new codepair(0x03cc, 0x0116), /*                   Eabovedot TIN CAPITAL LETTER E WITH DOT ABOVE */
    new codepair(0x03cf, 0x012a), /*                     Imacron TIN CAPITAL LETTER I WITH MACRON */
    new codepair(0x03d1, 0x0145), /*                    Ncedilla TIN CAPITAL LETTER N WITH CEDILLA */
    new codepair(0x03d2, 0x014c), /*                     Omacron TIN CAPITAL LETTER O WITH MACRON */
    new codepair(0x03d3, 0x0136), /*                    Kcedilla TIN CAPITAL LETTER K WITH CEDILLA */
    new codepair(0x03d9, 0x0172), /*                     Uogonek TIN CAPITAL LETTER U WITH OGONEK */
    new codepair(0x03dd, 0x0168), /*                      Utilde TIN CAPITAL LETTER U WITH TILDE */
    new codepair(0x03de, 0x016a), /*                     Umacron TIN CAPITAL LETTER U WITH MACRON */
    new codepair(0x03e0, 0x0101), /*                     amacron TIN SMALL LETTER A WITH MACRON */
    new codepair(0x03e7, 0x012f), /*                     iogonek TIN SMALL LETTER I WITH OGONEK */
    new codepair(0x03ec, 0x0117), /*                   eabovedot TIN SMALL LETTER E WITH DOT ABOVE */
    new codepair(0x03ef, 0x012b), /*                     imacron TIN SMALL LETTER I WITH MACRON */
    new codepair(0x03f1, 0x0146), /*                    ncedilla TIN SMALL LETTER N WITH CEDILLA */
    new codepair(0x03f2, 0x014d), /*                     omacron TIN SMALL LETTER O WITH MACRON */
    new codepair(0x03f3, 0x0137), /*                    kcedilla TIN SMALL LETTER K WITH CEDILLA */
    new codepair(0x03f9, 0x0173), /*                     uogonek TIN SMALL LETTER U WITH OGONEK */
    new codepair(0x03fd, 0x0169), /*                      utilde TIN SMALL LETTER U WITH TILDE */
    new codepair(0x03fe, 0x016b), /*                     umacron TIN SMALL LETTER U WITH MACRON */
    new codepair(0x047e, 0x203e), /*                    overline VERLINE */
    new codepair(0x04a1, 0x3002), /*               kana_fullstop DEOGRAPHIC FULL STOP */
    new codepair(0x04a2, 0x300c), /*         kana_openingbracket EFT CORNER BRACKET */
    new codepair(0x04a3, 0x300d), /*         kana_closingbracket IGHT CORNER BRACKET */
    new codepair(0x04a4, 0x3001), /*                  kana_comma DEOGRAPHIC COMMA */
    new codepair(0x04a5, 0x30fb), /*            kana_conjunctive ATAKANA MIDDLE DOT */
    new codepair(0x04a6, 0x30f2), /*                     kana_WO ATAKANA LETTER WO */
    new codepair(0x04a7, 0x30a1), /*                      kana_a ATAKANA LETTER SMALL A */
    new codepair(0x04a8, 0x30a3), /*                      kana_i ATAKANA LETTER SMALL I */
    new codepair(0x04a9, 0x30a5), /*                      kana_u ATAKANA LETTER SMALL U */
    new codepair(0x04aa, 0x30a7), /*                      kana_e ATAKANA LETTER SMALL E */
    new codepair(0x04ab, 0x30a9), /*                      kana_o ATAKANA LETTER SMALL O */
    new codepair(0x04ac, 0x30e3), /*                     kana_ya ATAKANA LETTER SMALL YA */
    new codepair(0x04ad, 0x30e5), /*                     kana_yu ATAKANA LETTER SMALL YU */
    new codepair(0x04ae, 0x30e7), /*                     kana_yo ATAKANA LETTER SMALL YO */
    new codepair(0x04af, 0x30c3), /*                    kana_tsu ATAKANA LETTER SMALL TU */
    new codepair(0x04b0, 0x30fc), /*              prolongedsound ATAKANA-HIRAGANA PROLONGED SOUND MARK */
    new codepair(0x04b1, 0x30a2), /*                      kana_A ATAKANA LETTER A */
    new codepair(0x04b2, 0x30a4), /*                      kana_I ATAKANA LETTER I */
    new codepair(0x04b3, 0x30a6), /*                      kana_U ATAKANA LETTER U */
    new codepair(0x04b4, 0x30a8), /*                      kana_E ATAKANA LETTER E */
    new codepair(0x04b5, 0x30aa), /*                      kana_O ATAKANA LETTER O */
    new codepair(0x04b6, 0x30ab), /*                     kana_KA ATAKANA LETTER KA */
    new codepair(0x04b7, 0x30ad), /*                     kana_KI ATAKANA LETTER KI */
    new codepair(0x04b8, 0x30af), /*                     kana_KU ATAKANA LETTER KU */
    new codepair(0x04b9, 0x30b1), /*                     kana_KE ATAKANA LETTER KE */
    new codepair(0x04ba, 0x30b3), /*                     kana_KO ATAKANA LETTER KO */
    new codepair(0x04bb, 0x30b5), /*                     kana_SA ATAKANA LETTER SA */
    new codepair(0x04bc, 0x30b7), /*                    kana_SHI ATAKANA LETTER SI */
    new codepair(0x04bd, 0x30b9), /*                     kana_SU ATAKANA LETTER SU */
    new codepair(0x04be, 0x30bb), /*                     kana_SE ATAKANA LETTER SE */
    new codepair(0x04bf, 0x30bd), /*                     kana_SO ATAKANA LETTER SO */
    new codepair(0x04c0, 0x30bf), /*                     kana_TA ATAKANA LETTER TA */
    new codepair(0x04c1, 0x30c1), /*                    kana_CHI ATAKANA LETTER TI */
    new codepair(0x04c2, 0x30c4), /*                    kana_TSU ATAKANA LETTER TU */
    new codepair(0x04c3, 0x30c6), /*                     kana_TE ATAKANA LETTER TE */
    new codepair(0x04c4, 0x30c8), /*                     kana_TO ATAKANA LETTER TO */
    new codepair(0x04c5, 0x30ca), /*                     kana_NA ATAKANA LETTER NA */
    new codepair(0x04c6, 0x30cb), /*                     kana_NI ATAKANA LETTER NI */
    new codepair(0x04c7, 0x30cc), /*                     kana_NU NA LETTER NU */
    new codepair(0x04c8, 0x30cd), /*                     kana_NE ATAKANA LETTER NE */
    new codepair(0x04c9, 0x30ce), /*                     kana_NO ATAKANA LETTER NO */
    new codepair(0x04ca, 0x30cf), /*                     kana_HA ATAKANA LETTER HA */
    new codepair(0x04cb, 0x30d2), /*                     kana_HI ATAKANA LETTER HI */
    new codepair(0x04cc, 0x30d5), /*                     kana_FU ATAKANA LETTER HU */
    new codepair(0x04cd, 0x30d8), /*                     kana_HE ATAKANA LETTER HE */
    new codepair(0x04ce, 0x30db), /*                     kana_HO ATAKANA LETTER HO */
    new codepair(0x04cf, 0x30de), /*                     kana_MA ATAKANA LETTER MA */
    new codepair(0x04d0, 0x30df), /*                     kana_MI ATAKANA LETTER MI */
    new codepair(0x04d1, 0x30e0), /*                     kana_MU ATAKANA LETTER MU */
    new codepair(0x04d2, 0x30e1), /*                     kana_ME ATAKANA LETTER ME */
    new codepair(0x04d3, 0x30e2), /*                     kana_MO ATAKANA LETTER MO */
    new codepair(0x04d4, 0x30e4), /*                     kana_YA ATAKANA LETTER YA */
    new codepair(0x04d5, 0x30e6), /*                     kana_YU ATAKANA LETTER YU */
    new codepair(0x04d6, 0x30e8), /*                     kana_YO ATAKANA LETTER YO */
    new codepair(0x04d7, 0x30e9), /*                     kana_RA ATAKANA LETTER RA */
    new codepair(0x04d8, 0x30ea), /*                     kana_RI ATAKANA LETTER RI */
    new codepair(0x04d9, 0x30eb), /*                     kana_RU ATAKANA LETTER RU */
    new codepair(0x04da, 0x30ec), /*                     kana_RE NA LETTER RE */
    new codepair(0x04db, 0x30ed), /*                     kana_RO ATAKANA LETTER RO */
    new codepair(0x04dc, 0x30ef), /*                     kana_WA ATAKANA LETTER WA */
    new codepair(0x04dd, 0x30f3), /*                      kana_N ATAKANA LETTER N */
    new codepair(0x04de, 0x309b), /*                 voicedsound ATAKANA-HIRAGANA VOICED SOUND MARK */
    new codepair(0x04df, 0x309c), /*             semivoicedsound ATAKANA-HIRAGANA SEMI-VOICED SOUND MARK */
    new codepair(0x05ac, 0x060c), /*                Arabic_comma ABIC COMMA */
    new codepair(0x05bb, 0x061b), /*            Arabic_semicolon ABIC SEMICOLON */
    new codepair(0x05bf, 0x061f), /*        Arabic_question_mark ABIC QUESTION MARK */
    new codepair(0x05c1, 0x0621), /*                Arabic_hamza ABIC LETTER HAMZA */
    new codepair(0x05c2, 0x0622), /*          Arabic_maddaonalef ABIC LETTER ALEF WITH MADDA ABOVE */
    new codepair(0x05c3, 0x0623), /*          Arabic_hamzaonalef ABIC LETTER ALEF WITH HAMZA ABOVE */
    new codepair(0x05c4, 0x0624), /*           Arabic_hamzaonwaw ABIC LETTER WAW WITH HAMZA ABOVE */
    new codepair(0x05c5, 0x0625), /*       Arabic_hamzaunderalef ABIC LETTER ALEF WITH HAMZA BELOW */
    new codepair(0x05c6, 0x0626), /*           Arabic_hamzaonyeh ABIC LETTER YEH WITH HAMZA ABOVE */
    new codepair(0x05c7, 0x0627), /*                 Arabic_alef ABIC LETTER ALEF */
    new codepair(0x05c8, 0x0628), /*                  Arabic_beh ABIC LETTER BEH */
    new codepair(0x05c9, 0x0629), /*           Arabic_tehmarbuta ABIC LETTER TEH MARBUTA */
    new codepair(0x05ca, 0x062a), /*                  Arabic_teh ABIC LETTER TEH */
    new codepair(0x05cb, 0x062b), /*                 Arabic_theh ABIC LETTER THEH */
    new codepair(0x05cc, 0x062c), /*                 Arabic_jeem LETTER JEEM */
    new codepair(0x05cd, 0x062d), /*                  Arabic_hah ABIC LETTER HAH */
    new codepair(0x05ce, 0x062e), /*                 Arabic_khah ABIC LETTER KHAH */
    new codepair(0x05cf, 0x062f), /*                  Arabic_dal ABIC LETTER DAL */
    new codepair(0x05d0, 0x0630), /*                 Arabic_thal ABIC LETTER THAL */
    new codepair(0x05d1, 0x0631), /*                   Arabic_ra ABIC LETTER REH */
    new codepair(0x05d2, 0x0632), /*                 Arabic_zain ABIC LETTER ZAIN */
    new codepair(0x05d3, 0x0633), /*                 Arabic_seen ABIC LETTER SEEN */
    new codepair(0x05d4, 0x0634), /*                Arabic_sheen ABIC LETTER SHEEN */
    new codepair(0x05d5, 0x0635), /*                  Arabic_sad ABIC LETTER SAD */
    new codepair(0x05d6, 0x0636), /*                  Arabic_dad ABIC LETTER DAD */
    new codepair(0x05d7, 0x0637), /*                  Arabic_tah ABIC LETTER TAH */
    new codepair(0x05d8, 0x0638), /*                  Arabic_zah ABIC LETTER ZAH */
    new codepair(0x05d9, 0x0639), /*                  Arabic_ain ABIC LETTER AIN */
    new codepair(0x05da, 0x063a), /*                Arabic_ghain ABIC LETTER GHAIN */
    new codepair(0x05e0, 0x0640), /*              Arabic_tatweel ABIC TATWEEL */
    new codepair(0x05e1, 0x0641), /*                  Arabic_feh ABIC LETTER FEH */
    new codepair(0x05e2, 0x0642), /*                  Arabic_qaf ABIC LETTER QAF */
    new codepair(0x05e3, 0x0643), /*                  Arabic_kaf ABIC LETTER KAF */
    new codepair(0x05e4, 0x0644), /*                  Arabic_lam ABIC LETTER LAM */
    new codepair(0x05e5, 0x0645), /*                 Arabic_meem ABIC LETTER MEEM */
    new codepair(0x05e6, 0x0646), /*                 Arabic_noon ABIC LETTER NOON */
    new codepair(0x05e7, 0x0647), /*                   Arabic_ha ABIC LETTER HEH */
    new codepair(0x05e8, 0x0648), /*                  Arabic_waw ABIC LETTER WAW */
    new codepair(0x05e9, 0x0649), /*          Arabic_alefmaksura ABIC LETTER ALEF MAKSURA */
    new codepair(0x05ea, 0x064a), /*                  Arabic_yeh ABIC LETTER YEH */
    new codepair(0x05eb, 0x064b), /*             Arabic_fathatan ABIC FATHATAN */
    new codepair(0x05ec, 0x064c), /*             Arabic_dammatan ABIC DAMMATAN */
    new codepair(0x05ed, 0x064d), /*             Arabic_kasratan ABIC KASRATAN */
    new codepair(0x05ee, 0x064e), /*                Arabic_fatha ABIC FATHA */
    new codepair(0x05ef, 0x064f), /*                Arabic_damma ABIC DAMMA */
    new codepair(0x05f0, 0x0650), /*                Arabic_kasra ABIC KASRA */
    new codepair(0x05f1, 0x0651), /*               Arabic_shadda ABIC SHADDA */
    new codepair(0x05f2, 0x0652), /*                Arabic_sukun SUKUN */
    new codepair(0x06a1, 0x0452), /*                 Serbian_dje RILLIC SMALL LETTER DJE */
    new codepair(0x06a2, 0x0453), /*               Macedonia_gje RILLIC SMALL LETTER GJE */
    new codepair(0x06a3, 0x0451), /*                 Cyrillic_io RILLIC SMALL LETTER IO */
    new codepair(0x06a4, 0x0454), /*                Ukrainian_ie RILLIC SMALL LETTER UKRAINIAN IE */
    new codepair(0x06a5, 0x0455), /*               Macedonia_dse RILLIC SMALL LETTER DZE */
    new codepair(0x06a6, 0x0456), /*                 Ukrainian_i RILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I */
    new codepair(0x06a7, 0x0457), /*                Ukrainian_yi RILLIC SMALL LETTER YI */
    new codepair(0x06a8, 0x0458), /*                 Cyrillic_je RILLIC SMALL LETTER JE */
    new codepair(0x06a9, 0x0459), /*                Cyrillic_lje RILLIC SMALL LETTER LJE */
    new codepair(0x06aa, 0x045a), /*                Cyrillic_nje RILLIC SMALL LETTER NJE */
    new codepair(0x06ab, 0x045b), /*                Serbian_tshe RILLIC SMALL LETTER TSHE */
    new codepair(0x06ac, 0x045c), /*               Macedonia_kje RILLIC SMALL LETTER KJE */
    new codepair(0x06ae, 0x045e), /*         Byelorussian_shortu RILLIC SMALL LETTER SHORT U */
    new codepair(0x06af, 0x045f), /*               Cyrillic_dzhe RILLIC SMALL LETTER DZHE */
    new codepair(0x06b0, 0x2116), /*                  numerosign UMERO SIGN */
    new codepair(0x06b1, 0x0402), /*                 Serbian_DJE RILLIC CAPITAL LETTER DJE */
    new codepair(0x06b2, 0x0403), /*               Macedonia_GJE RILLIC CAPITAL LETTER GJE */
    new codepair(0x06b3, 0x0401), /*                 Cyrillic_IO RILLIC CAPITAL LETTER IO */
    new codepair(0x06b4, 0x0404), /*                Ukrainian_IE RILLIC CAPITAL LETTER UKRAINIAN IE */
    new codepair(0x06b5, 0x0405), /*               Macedonia_DSE RILLIC CAPITAL LETTER DZE */
    new codepair(0x06b6, 0x0406), /*                 Ukrainian_I RILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I */
    new codepair(0x06b7, 0x0407), /*                Ukrainian_YI RILLIC CAPITAL LETTER YI */
    new codepair(0x06b8, 0x0408), /*                 Cyrillic_JE RILLIC CAPITAL LETTER JE */
    new codepair(0x06b9, 0x0409), /*                Cyrillic_LJE RILLIC CAPITAL LETTER LJE */
    new codepair(0x06ba, 0x040a), /*                Cyrillic_NJE RILLIC CAPITAL LETTER NJE */
    new codepair(0x06bb, 0x040b), /*                Serbian_TSHE RILLIC CAPITAL LETTER TSHE */
    new codepair(0x06bc, 0x040c), /*               Macedonia_KJE RILLIC CAPITAL LETTER KJE */
    new codepair(0x06be, 0x040e), /*         Byelorussian_SHORTU RILLIC CAPITAL LETTER SHORT U */
    new codepair(0x06bf, 0x040f), /*               Cyrillic_DZHE RILLIC CAPITAL LETTER DZHE */
    new codepair(0x06c0, 0x044e), /*                 Cyrillic_yu RILLIC SMALL LETTER YU */
    new codepair(0x06c1, 0x0430), /*                  Cyrillic_a RILLIC SMALL LETTER A */
    new codepair(0x06c2, 0x0431), /*                 Cyrillic_be RILLIC SMALL LETTER BE */
    new codepair(0x06c3, 0x0446), /*                Cyrillic_tse RILLIC SMALL LETTER TSE */
    new codepair(0x06c4, 0x0434), /*                 Cyrillic_de RILLIC SMALL LETTER DE */
    new codepair(0x06c5, 0x0435), /*                 Cyrillic_ie RILLIC SMALL LETTER IE */
    new codepair(0x06c6, 0x0444), /*                 Cyrillic_ef RILLIC SMALL LETTER EF */
    new codepair(0x06c7, 0x0433), /*                Cyrillic_ghe RILLIC SMALL LETTER GHE */
    new codepair(0x06c8, 0x0445), /*                 Cyrillic_ha RILLIC SMALL LETTER HA */
    new codepair(0x06c9, 0x0438), /*                  Cyrillic_i RILLIC SMALL LETTER I */
    new codepair(0x06ca, 0x0439), /*             Cyrillic_shorti RILLIC SMALL LETTER SHORT I */
    new codepair(0x06cb, 0x043a), /*                 Cyrillic_ka RILLIC SMALL LETTER KA */
    new codepair(0x06cc, 0x043b), /*                 Cyrillic_el RILLIC SMALL LETTER EL */
    new codepair(0x06cd, 0x043c), /*                 Cyrillic_em RILLIC SMALL LETTER EM */
    new codepair(0x06ce, 0x043d), /*                 Cyrillic_en RILLIC SMALL LETTER EN */
    new codepair(0x06cf, 0x043e), /*                  Cyrillic_o RILLIC SMALL LETTER O */
    new codepair(0x06d0, 0x043f), /*                 Cyrillic_pe RILLIC SMALL LETTER PE */
    new codepair(0x06d1, 0x044f), /*                 Cyrillic_ya RILLIC SMALL LETTER YA */
    new codepair(0x06d2, 0x0440), /*                 Cyrillic_er RILLIC SMALL LETTER ER */
    new codepair(0x06d3, 0x0441), /*                 Cyrillic_es RILLIC SMALL LETTER ES */
    new codepair(0x06d4, 0x0442), /*                 Cyrillic_te RILLIC SMALL LETTER TE */
    new codepair(0x06d5, 0x0443), /*                  Cyrillic_u RILLIC SMALL LETTER U */
    new codepair(0x06d6, 0x0436), /*                Cyrillic_zhe RILLIC SMALL LETTER ZHE */
    new codepair(0x06d7, 0x0432), /*                 Cyrillic_ve RILLIC SMALL LETTER VE */
    new codepair(0x06d8, 0x044c), /*           Cyrillic_softsign RILLIC SMALL LETTER SOFT SIGN */
    new codepair(0x06d9, 0x044b), /*               Cyrillic_yeru RILLIC SMALL LETTER YERU */
    new codepair(0x06da, 0x0437), /*                 Cyrillic_ze RILLIC SMALL LETTER ZE */
    new codepair(0x06db, 0x0448), /*                Cyrillic_sha RILLIC SMALL LETTER SHA */
    new codepair(0x06dc, 0x044d), /*                  Cyrillic_e RILLIC SMALL LETTER E */
    new codepair(0x06dd, 0x0449), /*              Cyrillic_shcha RILLIC SMALL LETTER SHCHA */
    new codepair(0x06de, 0x0447), /*                Cyrillic_che RILLIC SMALL LETTER CHE */
    new codepair(0x06df, 0x044a), /*           Cyrillic_hardsign RILLIC SMALL LETTER HARD SIGN */
    new codepair(0x06e0, 0x042e), /*                 Cyrillic_YU RILLIC CAPITAL LETTER YU */
    new codepair(0x06e1, 0x0410), /*                  Cyrillic_A RILLIC CAPITAL LETTER A */
    new codepair(0x06e2, 0x0411), /*                 Cyrillic_BE RILLIC CAPITAL LETTER BE */
    new codepair(0x06e3, 0x0426), /*                Cyrillic_TSE C CAPITAL LETTER TSE */
    new codepair(0x06e4, 0x0414), /*                 Cyrillic_DE RILLIC CAPITAL LETTER DE */
    new codepair(0x06e5, 0x0415), /*                 Cyrillic_IE RILLIC CAPITAL LETTER IE */
    new codepair(0x06e6, 0x0424), /*                 Cyrillic_EF RILLIC CAPITAL LETTER EF */
    new codepair(0x06e7, 0x0413), /*                Cyrillic_GHE RILLIC CAPITAL LETTER GHE */
    new codepair(0x06e8, 0x0425), /*                 Cyrillic_HA RILLIC CAPITAL LETTER HA */
    new codepair(0x06e9, 0x0418), /*                  Cyrillic_I RILLIC CAPITAL LETTER I */
    new codepair(0x06ea, 0x0419), /*             Cyrillic_SHORTI RILLIC CAPITAL LETTER SHORT I */
    new codepair(0x06eb, 0x041a), /*                 Cyrillic_KA RILLIC CAPITAL LETTER KA */
    new codepair(0x06ec, 0x041b), /*                 Cyrillic_EL RILLIC CAPITAL LETTER EL */
    new codepair(0x06ed, 0x041c), /*                 Cyrillic_EM RILLIC CAPITAL LETTER EM */
    new codepair(0x06ee, 0x041d), /*                 Cyrillic_EN RILLIC CAPITAL LETTER EN */
    new codepair(0x06ef, 0x041e), /*                  Cyrillic_O RILLIC CAPITAL LETTER O */
    new codepair(0x06f0, 0x041f), /*                 Cyrillic_PE RILLIC CAPITAL LETTER PE */
    new codepair(0x06f1, 0x042f), /*                 Cyrillic_YA RILLIC CAPITAL LETTER YA */
    new codepair(0x06f2, 0x0420), /*                 Cyrillic_ER RILLIC CAPITAL LETTER ER */
    new codepair(0x06f3, 0x0421), /*                 Cyrillic_ES RILLIC CAPITAL LETTER ES */
    new codepair(0x06f4, 0x0422), /*                 Cyrillic_TE RILLIC CAPITAL LETTER TE */
    new codepair(0x06f5, 0x0423), /*                  Cyrillic_U RILLIC CAPITAL LETTER U */
    new codepair(0x06f6, 0x0416), /*                Cyrillic_ZHE RILLIC CAPITAL LETTER ZHE */
    new codepair(0x06f7, 0x0412), /*                 Cyrillic_VE RILLIC CAPITAL LETTER VE */
    new codepair(0x06f8, 0x042c), /*           Cyrillic_SOFTSIGN RILLIC CAPITAL LETTER SOFT SIGN */
    new codepair(0x06f9, 0x042b), /*               Cyrillic_YERU RILLIC CAPITAL LETTER YERU */
    new codepair(0x06fa, 0x0417), /*                 Cyrillic_ZE RILLIC CAPITAL LETTER ZE */
    new codepair(0x06fb, 0x0428), /*                Cyrillic_SHA RILLIC CAPITAL LETTER SHA */
    new codepair(0x06fc, 0x042d), /*                  Cyrillic_E RILLIC CAPITAL LETTER E */
    new codepair(0x06fd, 0x0429), /*              Cyrillic_SHCHA RILLIC CAPITAL LETTER SHCHA */
    new codepair(0x06fe, 0x0427), /*                Cyrillic_CHE RILLIC CAPITAL LETTER CHE */
    new codepair(0x06ff, 0x042a), /*           Cyrillic_HARDSIGN RILLIC CAPITAL LETTER HARD SIGN */
    new codepair(0x07a1, 0x0386), /*           Greek_ALPHAaccent EEK CAPITAL LETTER ALPHA WITH TONOS */
    new codepair(0x07a2, 0x0388), /*         Greek_EPSILONaccent EEK CAPITAL LETTER EPSILON WITH TONOS */
    new codepair(0x07a3, 0x0389), /*             Greek_ETAaccent EEK CAPITAL LETTER ETA WITH TONOS */
    new codepair(0x07a4, 0x038a), /*            Greek_IOTAaccent EEK CAPITAL LETTER IOTA WITH TONOS */
    new codepair(0x07a5, 0x03aa), /*         Greek_IOTAdiaeresis EEK CAPITAL LETTER IOTA WITH DIALYTIKA */
    new codepair(0x07a7, 0x038c), /*         Greek_OMICRONaccent EEK CAPITAL LETTER OMICRON WITH TONOS */
    new codepair(0x07a8, 0x038e), /*         Greek_UPSILONaccent EEK CAPITAL LETTER UPSILON WITH TONOS */
    new codepair(0x07a9, 0x03ab), /*       Greek_UPSILONdieresis EEK CAPITAL LETTER UPSILON WITH DIALYTIKA */
    new codepair(0x07ab, 0x038f), /*           Greek_OMEGAaccent EEK CAPITAL LETTER OMEGA WITH TONOS */
    new codepair(0x07ae, 0x0385), /*        Greek_accentdieresis EEK DIALYTIKA TONOS */
    new codepair(0x07af, 0x2015), /*              Greek_horizbar ORIZONTAL BAR */
    new codepair(0x07b1, 0x03ac), /*           Greek_alphaaccent EEK SMALL LETTER ALPHA WITH TONOS */
    new codepair(0x07b2, 0x03ad), /*         Greek_epsilonaccent MALL LETTER EPSILON WITH TONOS */
    new codepair(0x07b3, 0x03ae), /*             Greek_etaaccent EEK SMALL LETTER ETA WITH TONOS */
    new codepair(0x07b4, 0x03af), /*            Greek_iotaaccent EEK SMALL LETTER IOTA WITH TONOS */
    new codepair(0x07b5, 0x03ca), /*          Greek_iotadieresis EEK SMALL LETTER IOTA WITH DIALYTIKA */
    new codepair(0x07b6, 0x0390), /*    Greek_iotaaccentdieresis EEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS */
    new codepair(0x07b7, 0x03cc), /*         Greek_omicronaccent EEK SMALL LETTER OMICRON WITH TONOS */
    new codepair(0x07b8, 0x03cd), /*         Greek_upsilonaccent EEK SMALL LETTER UPSILON WITH TONOS */
    new codepair(0x07b9, 0x03cb), /*       Greek_upsilondieresis EEK SMALL LETTER UPSILON WITH DIALYTIKA */
    new codepair(0x07ba, 0x03b0), /* Greek_upsilonaccentdieresis EEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS */
    new codepair(0x07bb, 0x03ce), /*           Greek_omegaaccent EEK SMALL LETTER OMEGA WITH TONOS */
    new codepair(0x07c1, 0x0391), /*                 Greek_ALPHA EEK CAPITAL LETTER ALPHA */
    new codepair(0x07c2, 0x0392), /*                  Greek_BETA EEK CAPITAL LETTER BETA */
    new codepair(0x07c3, 0x0393), /*                 Greek_GAMMA EEK CAPITAL LETTER GAMMA */
    new codepair(0x07c4, 0x0394), /*                 Greek_DELTA EEK CAPITAL LETTER DELTA */
    new codepair(0x07c5, 0x0395), /*               Greek_EPSILON EEK CAPITAL LETTER EPSILON */
    new codepair(0x07c6, 0x0396), /*                  Greek_ZETA EEK CAPITAL LETTER ZETA */
    new codepair(0x07c7, 0x0397), /*                   Greek_ETA EEK CAPITAL LETTER ETA */
    new codepair(0x07c8, 0x0398), /*                 Greek_THETA EEK CAPITAL LETTER THETA */
    new codepair(0x07c9, 0x0399), /*                  Greek_IOTA EEK CAPITAL LETTER IOTA */
    new codepair(0x07ca, 0x039a), /*                 Greek_KAPPA EEK CAPITAL LETTER KAPPA */
    new codepair(0x07cb, 0x039b), /*                Greek_LAMBDA EEK CAPITAL LETTER LAMDA */
    new codepair(0x07cc, 0x039c), /*                    Greek_MU EEK CAPITAL LETTER MU */
    new codepair(0x07cd, 0x039d), /*                    Greek_NU EEK CAPITAL LETTER NU */
    new codepair(0x07ce, 0x039e), /*                    Greek_XI EEK CAPITAL LETTER XI */
    new codepair(0x07cf, 0x039f), /*               Greek_OMICRON EEK CAPITAL LETTER OMICRON */
    new codepair(0x07d0, 0x03a0), /*                    Greek_PI EEK CAPITAL LETTER PI */
    new codepair(0x07d1, 0x03a1), /*                   Greek_RHO EEK CAPITAL LETTER RHO */
    new codepair(0x07d2, 0x03a3), /*                 Greek_SIGMA EEK CAPITAL LETTER SIGMA */
    new codepair(0x07d4, 0x03a4), /*                   Greek_TAU EEK CAPITAL LETTER TAU */
    new codepair(0x07d5, 0x03a5), /*               Greek_UPSILON EEK CAPITAL LETTER UPSILON */
    new codepair(0x07d6, 0x03a6), /*                   Greek_PHI EEK CAPITAL LETTER PHI */
    new codepair(0x07d7, 0x03a7), /*                   Greek_CHI EEK CAPITAL LETTER CHI */
    new codepair(0x07d8, 0x03a8), /*                   Greek_PSI EEK CAPITAL LETTER PSI */
    new codepair(0x07d9, 0x03a9), /*                 Greek_OMEGA EEK CAPITAL LETTER OMEGA */
    new codepair(0x07e1, 0x03b1), /*                 Greek_alpha EEK SMALL LETTER ALPHA */
    new codepair(0x07e2, 0x03b2), /*                  Greek_beta EEK SMALL LETTER BETA */
    new codepair(0x07e3, 0x03b3), /*                 Greek_gamma EEK SMALL LETTER GAMMA */
    new codepair(0x07e4, 0x03b4), /*                 Greek_delta EEK SMALL LETTER DELTA */
    new codepair(0x07e5, 0x03b5), /*               Greek_epsilon EEK SMALL LETTER EPSILON */
    new codepair(0x07e6, 0x03b6), /*                  Greek_zeta EEK SMALL LETTER ZETA */
    new codepair(0x07e7, 0x03b7), /*                   Greek_eta EEK SMALL LETTER ETA */
    new codepair(0x07e8, 0x03b8), /*                 Greek_theta EEK SMALL LETTER THETA */
    new codepair(0x07e9, 0x03b9), /*                  Greek_iota EEK SMALL LETTER IOTA */
    new codepair(0x07ea, 0x03ba), /*                 Greek_kappa EEK SMALL LETTER KAPPA */
    new codepair(0x07eb, 0x03bb), /*                Greek_lambda EEK SMALL LETTER LAMDA */
    new codepair(0x07ec, 0x03bc), /*                    Greek_mu EEK SMALL LETTER MU */
    new codepair(0x07ed, 0x03bd), /*                    Greek_nu EEK SMALL LETTER NU */
    new codepair(0x07ee, 0x03be), /*                    Greek_xi EEK SMALL LETTER XI */
    new codepair(0x07ef, 0x03bf), /*               Greek_omicron EEK SMALL LETTER OMICRON */
    new codepair(0x07f0, 0x03c0), /*                    Greek_pi EEK SMALL LETTER PI */
    new codepair(0x07f1, 0x03c1), /*                   Greek_rho EEK SMALL LETTER RHO */
    new codepair(0x07f2, 0x03c3), /*                 Greek_sigma EEK SMALL LETTER SIGMA */
    new codepair(0x07f3, 0x03c2), /*       Greek_finalsmallsigma EEK SMALL LETTER FINAL SIGMA */
    new codepair(0x07f4, 0x03c4), /*                   Greek_tau EEK SMALL LETTER TAU */
    new codepair(0x07f5, 0x03c5), /*               Greek_upsilon EEK SMALL LETTER UPSILON */
    new codepair(0x07f6, 0x03c6), /*                   Greek_phi EEK SMALL LETTER PHI */
    new codepair(0x07f7, 0x03c7), /*                   Greek_chi EEK SMALL LETTER CHI */
    new codepair(0x07f8, 0x03c8), /*                   Greek_psi EEK SMALL LETTER PSI */
    new codepair(0x07f9, 0x03c9), /*                 Greek_omega EEK SMALL LETTER OMEGA */
    new codepair(0x08a1, 0x23b7), /*                 leftradical ?? */
    new codepair(0x08a2, 0x250c), /*              topleftradical OX DRAWINGS LIGHT DOWN AND RIGHT */
    new codepair(0x08a3, 0x2500), /*              horizconnector OX DRAWINGS LIGHT HORIZONTAL */
    new codepair(0x08a4, 0x2320), /*                 topintegral OP HALF INTEGRAL */
    new codepair(0x08a5, 0x2321), /*                 botintegral OTTOM HALF INTEGRAL */
    new codepair(0x08a6, 0x2502), /*               vertconnector OX DRAWINGS LIGHT VERTICAL */
    new codepair(0x08a7, 0x23a1), /*            topleftsqbracket ?? */
    new codepair(0x08a8, 0x23a3), /*            botleftsqbracket ?? */
    new codepair(0x08a9, 0x23a4), /*           toprightsqbracket ?? */
    new codepair(0x08aa, 0x23a6), /*           botrightsqbracket ?? */
    new codepair(0x08ab, 0x239b), /*               topleftparens ?? */
    new codepair(0x08ac, 0x239d), /*               botleftparens ?? */
    new codepair(0x08ad, 0x239e), /*              toprightparens ?? */
    new codepair(0x08ae, 0x23a0), /*              botrightparens ?? */
    new codepair(0x08af, 0x23a8), /*        leftmiddlecurlybrace ?? */
    new codepair(0x08b0, 0x23ac), /*       rightmiddlecurlybrace ?? */
  /*  0x08b1                          topleftsummation ? ??? */
  /*  0x08b2                          botleftsummation ? ??? */
  /*  0x08b3                 topvertsummationconnector ? ??? */
  /*  0x08b4                 botvertsummationconnector ? ??? */
  /*  0x08b5                         toprightsummation ? ??? */
  /*  0x08b6                         botrightsummation ? ??? */
  /*  0x08b7                      rightmiddlesummation ? ??? */
    new codepair(0x08bc, 0x2264), /*               lessthanequal ESS-THAN OR EQUAL TO */
    new codepair(0x08bd, 0x2260), /*                    notequal OT EQUAL TO */
    new codepair(0x08be, 0x2265), /*            greaterthanequal REATER-THAN OR EQUAL TO */
    new codepair(0x08bf, 0x222b), /*                    integral NTEGRAL */
    new codepair(0x08c0, 0x2234), /*                   therefore HEREFORE */
    new codepair(0x08c1, 0x221d), /*                   variation ROPORTIONAL TO */
    new codepair(0x08c2, 0x221e), /*                    infinity NFINITY */
    new codepair(0x08c5, 0x2207), /*                       nabla ABLA */
    new codepair(0x08c8, 0x223c), /*                 approximate ILDE OPERATOR */
    new codepair(0x08c9, 0x2243), /*                similarequal SYMPTOTICALLY EQUAL TO */
    new codepair(0x08cd, 0x21d4), /*                    ifonlyif EFT RIGHT DOUBLE ARROW */
    new codepair(0x08ce, 0x21d2), /*                     implies IGHTWARDS DOUBLE ARROW */
    new codepair(0x08cf, 0x2261), /*                   identical DENTICAL TO */
    new codepair(0x08d6, 0x221a), /*                     radical QUARE ROOT */
    new codepair(0x08da, 0x2282), /*                  includedin UBSET OF */
    new codepair(0x08db, 0x2283), /*                    includes UPERSET OF */
    new codepair(0x08dc, 0x2229), /*                intersection NTERSECTION */
    new codepair(0x08dd, 0x222a), /*                       union NION */
    new codepair(0x08de, 0x2227), /*                  logicaland OGICAL AND */
    new codepair(0x08df, 0x2228), /*                   logicalor OGICAL OR */
    new codepair(0x08ef, 0x2202), /*           partialderivative ARTIAL DIFFERENTIAL */
    new codepair(0x08f6, 0x0192), /*                    function TIN SMALL LETTER F WITH HOOK */
    new codepair(0x08fb, 0x2190), /*                   leftarrow EFTWARDS ARROW */
    new codepair(0x08fc, 0x2191), /*                     uparrow PWARDS ARROW */
    new codepair(0x08fd, 0x2192), /*                  rightarrow IGHTWARDS ARROW */
    new codepair(0x08fe, 0x2193), /*                   downarrow OWNWARDS ARROW */
  /*  0x09df                                     blank ? ??? */
    new codepair(0x09e0, 0x25c6), /*                soliddiamLACK DIAMOND */
    new codepair(0x09e1, 0x2592), /*                checkerboard MEDIUM SHADE */
    new codepair(0x09e2, 0x2409), /*                          ht YMBOL FOR HORIZONTAL TABULATION */
    new codepair(0x09e3, 0x240c), /*                          ff YMBOL FOR FORM FEED */
    new codepair(0x09e4, 0x240d), /*                          cr YMBOL FOR CARRIAGE RETURN */
    new codepair(0x09e5, 0x240a), /*                          lf YMBOL FOR LINE FEED */
    new codepair(0x09e8, 0x2424), /*                          nl YMBOL FOR NEWLINE */
    new codepair(0x09e9, 0x240b), /*                          vt YMBOL FOR VERTICAL TABULATION */
    new codepair(0x09ea, 0x2518), /*              lowrightcorner OX DRAWINGS LIGHT UP AND LEFT */
    new codepair(0x09eb, 0x2510), /*               uprightcorner OX DRAWINGS LIGHT DOWN AND LEFT */
    new codepair(0x09ec, 0x250c), /*                upleftcorner OX DRAWINGS LIGHT DOWN AND RIGHT */
    new codepair(0x09ed, 0x2514), /*               lowleftcorner OX DRAWINGS LIGHT UP AND RIGHT */
    new codepair(0x09ee, 0x253c), /*               crossinglines OX DRAWINGS LIGHT VERTICAL AND HORIZONTAL */
    new codepair(0x09ef, 0x23ba), /*              horizlinescan1 ORIZONTAL SCAN LINE-1 (Unicode 3.2 draft) */
    new codepair(0x09f0, 0x23bb), /*              horizlinescan3 ORIZONTAL SCAN LINE-3 (Unicode 3.2 draft) */
    new codepair(0x09f1, 0x2500), /*              horizlinescan5 OX DRAWINGS LIGHT HORIZONTAL */
    new codepair(0x09f2, 0x23bc), /*              horizlinescan7 ORIZONTAL SCAN LINE-7 (Unicode 3.2 draft) */
    new codepair(0x09f3, 0x23bd), /*              horizlinescan9 ORIZONTAL SCAN LINE-9 (Unicode 3.2 draft) */
    new codepair(0x09f4, 0x251c), /*                       leftt OX DRAWINGS LIGHT VERTICAL AND RIGHT */
    new codepair(0x09f5, 0x2524), /*                      rightt OX DRAWINGS LIGHT VERTICAL AND LEFT */
    new codepair(0x09f6, 0x2534), /*                        bott OX DRAWINGS LIGHT UP AND HORIZONTAL */
    new codepair(0x09f7, 0x252c), /*                        topt OX DRAWINGS LIGHT DOWN AND HORIZONTAL */
    new codepair(0x09f8, 0x2502), /*                     vertbar OX DRAWINGS LIGHT VERTICAL */
    new codepair(0x0aa1, 0x2003), /*                     emspace M SPACE */
    new codepair(0x0aa2, 0x2002), /*                     enspace N SPACE */
    new codepair(0x0aa3, 0x2004), /*                    em3space HREE-PER-EM SPACE */
    new codepair(0x0aa4, 0x2005), /*                    em4space OUR-PER-EM SPACE */
    new codepair(0x0aa5, 0x2007), /*                  digitspace IGURE SPACE */
    new codepair(0x0aa6, 0x2008), /*                  punctspace UNCTUATION SPACE */
    new codepair(0x0aa7, 0x2009), /*                   thinspace HIN SPACE */
    new codepair(0x0aa8, 0x200a), /*                   hairspace AIR SPACE */
    new codepair(0x0aa9, 0x2014), /*                      emdash M DASH */
    new codepair(0x0aaa, 0x2013), /*                      endash N DASH */
  /*  0x0aac                               signifblank ? ??? */
    new codepair(0x0aae, 0x2026), /*                    ellipHORIZONTAL ELLIPSIS */
    new codepair(0x0aaf, 0x2025), /*             doubbaselinedot WO DOT LEADER */
    new codepair(0x0ab0, 0x2153), /*                    onethird ULGAR FRACTION ONE THIRD */
    new codepair(0x0ab1, 0x2154), /*                   twothirds ULGAR FRACTION TWO THIRDS */
    new codepair(0x0ab2, 0x2155), /*                    onefifth ULGAR FRACTION ONE FIFTH */
    new codepair(0x0ab3, 0x2156), /*                   twofifths ULGAR FRACTION TWO FIFTHS */
    new codepair(0x0ab4, 0x2157), /*                 threefifths ULGAR FRACTION THREE FIFTHS */
    new codepair(0x0ab5, 0x2158), /*                  fourfifths ULGAR FRACTION FOUR FIFTHS */
    new codepair(0x0ab6, 0x2159), /*                    onesixth ULGAR FRACTION ONE SIXTH */
    new codepair(0x0ab7, 0x215a), /*                  fivesixths ULGAR FRACTION FIVE SIXTHS */
    new codepair(0x0ab8, 0x2105), /*                      careof ARE OF */
    new codepair(0x0abb, 0x2012), /*                     figdash IGURE DASH */
    new codepair(0x0abc, 0x2329), /*            leftanglebracket EFT-POINTING ANGLE BRACKET */
  /*  0x0abd                              decimalpoint ? ??? */
    new codepair(0x0abe, 0x232a), /*           rightanglebracket IGHT-POINTING ANGLE BRACKET */
  /*  0x0abf                                    marker ? ??? */
    new codepair(0x0ac3, 0x215b), /*                   oneeighth ULGAR FRACTION ONE EIGHTH */
    new codepair(0x0ac4, 0x215c), /*                threeeighths ULGAR FRACTION THREE EIGHTHS */
    new codepair(0x0ac5, 0x215d), /*                 fiveeighths ULGAR FRACTION FIVE EIGHTHS */
    new codepair(0x0ac6, 0x215e), /*                seveneighths ULGAR FRACTION SEVEN EIGHTHS */
    new codepair(0x0ac9, 0x2122), /*                   trademark RADE MARK SIGN */
    new codepair(0x0aca, 0x2613), /*               signaturemark ALTIRE */
  /*  0x0acb                         trademarkincircle ? ??? */
    new codepair(0x0acc, 0x25c1), /*            leftopentriangle HITE LEFT-POINTING TRIANGLE */
    new codepair(0x0acd, 0x25b7), /*           rightopentriangle HITE RIGHT-POINTING TRIANGLE */
    new codepair(0x0ace, 0x25cb), /*                emopencircle HITE CIRCLE */
    new codepair(0x0acf, 0x25af), /*             emopenrectangle HITE VERTICAL RECTANGLE */
    new codepair(0x0ad0, 0x2018), /*         leftsinglequotemark EFT SINGLE QUOTATION MARK */
    new codepair(0x0ad1, 0x2019), /*        rightsinglequotemark IGHT SINGLE QUOTATION MARK */
    new codepair(0x0ad2, 0x201c), /*         leftdoublequotemark EFT DOUBLE QUOTATION MARK */
    new codepair(0x0ad3, 0x201d), /*        rightdoublequotemark IGHT DOUBLE QUOTATION MARK */
    new codepair(0x0ad4, 0x211e), /*                prescription RESCRIPTION TAKE */
    new codepair(0x0ad6, 0x2032), /*                     minutes RIME */
    new codepair(0x0ad7, 0x2033), /*                     seconds OUBLE PRIME */
    new codepair(0x0ad9, 0x271d), /*                  latincross ATIN CROSS */
  /*  0x0ada                                  hexagram ? ??? */
    new codepair(0x0adb, 0x25ac), /*            filledrectbullet LACK RECTANGLE */
    new codepair(0x0adc, 0x25c0), /*         filledlefttribullet LACK LEFT-POINTING TRIANGLE */
    new codepair(0x0add, 0x25b6), /*        filledrighttribullet LACK RIGHT-POINTING TRIANGLE */
    new codepair(0x0ade, 0x25cf), /*              emfilledcircle LACK CIRCLE */
    new codepair(0x0adf, 0x25ae), /*                emfilledrect LACK VERTICAL RECTANGLE */
    new codepair(0x0ae0, 0x25e6), /*            enopencircbullet HITE BULLET */
    new codepair(0x0ae1, 0x25ab), /*          enopensquarebullet HITE SMALL SQUARE */
    new codepair(0x0ae2, 0x25ad), /*              openrectbullet HITE RECTANGLE */
    new codepair(0x0ae3, 0x25b3), /*             opentribulletup HITE UP-POINTING TRIANGLE */
    new codepair(0x0ae4, 0x25bd), /*           opentribulletdown HITE DOWN-POINTING TRIANGLE */
    new codepair(0x0ae5, 0x2606), /*                    openstar HITE STAR */
    new codepair(0x0ae6, 0x2022), /*          enfilledcircbullet ULLET */
    new codepair(0x0ae7, 0x25aa), /*            enfilledsqbullet LACK SMALL SQUARE */
    new codepair(0x0ae8, 0x25b2), /*           filledtribulletup LACK UP-POINTING TRIANGLE */
    new codepair(0x0ae9, 0x25bc), /*         filledtribulletdown LACK DOWN-POINTING TRIANGLE */
    new codepair(0x0aea, 0x261c), /*                 leftpointer HITE LEFT POINTING INDEX */
    new codepair(0x0aeb, 0x261e), /*                rightpointer HITE RIGHT POINTING INDEX */
    new codepair(0x0aec, 0x2663), /*                        club LACK CLUB SUIT */
    new codepair(0x0aed, 0x2666), /*                     diamond LACK DIAMOND SUIT */
    new codepair(0x0aee, 0x2665), /*                       heart LACK HEART SUIT */
    new codepair(0x0af0, 0x2720), /*                maltesecross ALTESE CROSS */
    new codepair(0x0af1, 0x2020), /*                      dagger AGGER */
    new codepair(0x0af2, 0x2021), /*                doubledagger OUBLE DAGGER */
    new codepair(0x0af3, 0x2713), /*                   checkmark HECK MARK */
    new codepair(0x0af4, 0x2717), /*                 ballotcross ALLOT X */
    new codepair(0x0af5, 0x266f), /*                musicalsharp USIC SHARP SIGN */
    new codepair(0x0af6, 0x266d), /*                 musicalflat USIC FLAT SIGN */
    new codepair(0x0af7, 0x2642), /*                  malesymbol ALE SIGN */
    new codepair(0x0af8, 0x2640), /*                femalesymbol EMALE SIGN */
    new codepair(0x0af9, 0x260e), /*                   telephone LACK TELEPHONE */
    new codepair(0x0afa, 0x2315), /*           telephonerecorder ELEPHONE RECORDER */
    new codepair(0x0afb, 0x2117), /*         phonographcopyright OUND RECORDING COPYRIGHT */
    new codepair(0x0afc, 0x2038), /*                       caret ARET */
    new codepair(0x0afd, 0x201a), /*          singlelowquotemark INGLE LOW-9 QUOTATION MARK */
    new codepair(0x0afe, 0x201e), /*          doublelowquotemark OUBLE LOW-9 QUOTATION MARK */
  /*  0x0aff                                    cursor ? ??? */
    new codepair(0x0ba3, 0x003c), /*                   leftcaret < LESS-THAN SIGN */
    new codepair(0x0ba6, 0x003e), /*                  rightcaret > GREATER-THAN SIGN */
    new codepair(0x0ba8, 0x2228), /*                   downcaret OGICAL OR */
    new codepair(0x0ba9, 0x2227), /*                     upcaret OGICAL AND */
    new codepair(0x0bc0, 0x00af), /*                     overbar CRON */
    new codepair(0x0bc2, 0x22a5), /*                    downtack P TACK */
    new codepair(0x0bc3, 0x2229), /*                      upshoe NTERSECTION */
    new codepair(0x0bc4, 0x230a), /*                   downstile EFT FLOOR */
    new codepair(0x0bc6, 0x005f), /*                    underbar  LINE */
    new codepair(0x0bca, 0x2218), /*                         jot ING OPERATOR */
    new codepair(0x0bcc, 0x2395), /*                        quad PL FUNCTIONAL SYMBOL QUAD */
    new codepair(0x0bce, 0x22a4), /*                      uptack OWN TACK */
    new codepair(0x0bcf, 0x25cb), /*                      circle HITE CIRCLE */
    new codepair(0x0bd3, 0x2308), /*                     upstile EFT CEILING */
    new codepair(0x0bd6, 0x222a), /*                    downshoe NION */
    new codepair(0x0bd8, 0x2283), /*                   rightshoe UPERSET OF */
    new codepair(0x0bda, 0x2282), /*                    leftshoe UBSET OF */
    new codepair(0x0bdc, 0x22a2), /*                    lefttack IGHT TACK */
    new codepair(0x0bfc, 0x22a3), /*                   righttack EFT TACK */
    new codepair(0x0cdf, 0x2017), /*        hebrew_doublelowline OUBLE LOW LINE */
    new codepair(0x0ce0, 0x05d0), /*                hebrew_aleph BREW LETTER ALEF */
    new codepair(0x0ce1, 0x05d1), /*                  hebrew_bet BREW LETTER BET */
    new codepair(0x0ce2, 0x05d2), /*                hebrew_gimel BREW LETTER GIMEL */
    new codepair(0x0ce3, 0x05d3), /*                hebrew_dalet BREW LETTER DALET */
    new codepair(0x0ce4, 0x05d4), /*                   hebrew_he BREW LETTER HE */
    new codepair(0x0ce5, 0x05d5), /*                  hebrew_waw BREW LETTER VAV */
    new codepair(0x0ce6, 0x05d6), /*                 hebrew_zain BREW LETTER ZAYIN */
    new codepair(0x0ce7, 0x05d7), /*                 hebrew_chet BREW LETTER HET */
    new codepair(0x0ce8, 0x05d8), /*                  hebrew_tet BREW LETTER TET */
    new codepair(0x0ce9, 0x05d9), /*                  hebrew_yod BREW LETTER YOD */
    new codepair(0x0cea, 0x05da), /*            hebrew_finalkaph BREW LETTER FINAL KAF */
    new codepair(0x0ceb, 0x05db), /*                 hebrew_kaph BREW LETTER KAF */
    new codepair(0x0cec, 0x05dc), /*                hebrew_lamed BREW LETTER LAMED */
    new codepair(0x0ced, 0x05dd), /*             hebrew_finalmem BREW LETTER FINAL MEM */
    new codepair(0x0cee, 0x05de), /*                  hebrew_mem BREW LETTER MEM */
    new codepair(0x0cef, 0x05df), /*             hebrew_finalnun BREW LETTER FINAL NUN */
    new codepair(0x0cf0, 0x05e0), /*                  hebrew_nun BREW LETTER NUN */
    new codepair(0x0cf1, 0x05e1), /*               hebrew_samech BREW LETTER SAMEKH */
    new codepair(0x0cf2, 0x05e2), /*                 hebrew_ayin BREW LETTER AYIN */
    new codepair(0x0cf3, 0x05e3), /*              hebrew_finalpe BREW LETTER FINAL PE */
    new codepair(0x0cf4, 0x05e4), /*                   hebrew_pe BREW LETTER PE */
    new codepair(0x0cf5, 0x05e5), /*            hebrew_finalzade BREW LETTER FINAL TSADI */
    new codepair(0x0cf6, 0x05e6), /*                 hebrew_zade BREW LETTER TSADI */
    new codepair(0x0cf7, 0x05e7), /*                 hebrew_qoph BREW LETTER QOF */
    new codepair(0x0cf8, 0x05e8), /*                 hebrew_resh BREW LETTER RESH */
    new codepair(0x0cf9, 0x05e9), /*                 hebrew_shin BREW LETTER SHIN */
    new codepair(0x0cfa, 0x05ea), /*                  hebrew_taw BREW LETTER TAV */
    new codepair(0x0da1, 0x0e01), /*                  Thai_kokai HAI CHARACTER KO KAI */
    new codepair(0x0da2, 0x0e02), /*                Thai_khokhai HAI CHARACTER KHO KHAI */
    new codepair(0x0da3, 0x0e03), /*               Thai_khokhuat HAI CHARACTER KHO KHUAT */
    new codepair(0x0da4, 0x0e04), /*               Thai_khokhwai HAI CHARACTER KHO KHWAI */
    new codepair(0x0da5, 0x0e05), /*                Thai_khokhon HAI CHARACTER KHO KHON */
    new codepair(0x0da6, 0x0e06), /*             Thai_khorakhang HAI CHARACTER KHO RAKHANG */
    new codepair(0x0da7, 0x0e07), /*                 Thai_ngongu HAI CHARACTER NGO NGU */
    new codepair(0x0da8, 0x0e08), /*                Thai_chochan HAI CHARACTER CHO CHAN */
    new codepair(0x0da9, 0x0e09), /*               Thai_choching HAI CHARACTER CHO CHING */
    new codepair(0x0daa, 0x0e0a), /*               Thai_chochang HAI CHARACTER CHO CHANG */
    new codepair(0x0dab, 0x0e0b), /*                   Thai_soso HAI CHARACTER SO SO */
    new codepair(0x0dac, 0x0e0c), /*                Thai_chochoe HAI CHARACTER CHO CHOE */
    new codepair(0x0dad, 0x0e0d), /*                 Thai_yoying HAI CHARACTER YO YING */
    new codepair(0x0dae, 0x0e0e), /*                Thai_dochada HAI CHARACTER DO CHADA */
    new codepair(0x0daf, 0x0e0f), /*                Thai_topatak HAI CHARACTER TO PATAK */
    new codepair(0x0db0, 0x0e10), /*                Thai_thothan HAI CHARACTER THO THAN */
    new codepair(0x0db1, 0x0e11), /*          Thai_thonangmontho HAI CHARACTER THO NANGMONTHO */
    new codepair(0x0db2, 0x0e12), /*             Thai_thophuthao HAI CHARACTER THO PHUTHAO */
    new codepair(0x0db3, 0x0e13), /*                  Thai_nonen HAI CHARACTER NO NEN */
    new codepair(0x0db4, 0x0e14), /*                  Thai_dodek HAI CHARACTER DO DEK */
    new codepair(0x0db5, 0x0e15), /*                  Thai_totao HAI CHARACTER TO TAO */
    new codepair(0x0db6, 0x0e16), /*               Thai_thothung HAI CHARACTER THO THUNG */
    new codepair(0x0db7, 0x0e17), /*              Thai_thothahan HAI CHARACTER THO THAHAN */
    new codepair(0x0db8, 0x0e18), /*               Thai_thothong HAI CHARACTER THO THONG */
    new codepair(0x0db9, 0x0e19), /*                   Thai_nonu HAI CHARACTER NO NU */
    new codepair(0x0dba, 0x0e1a), /*               Thai_bobaimai HAI CHARACTER BO BAIMAI */
    new codepair(0x0dbb, 0x0e1b), /*                  Thai_popla HAI CHARACTER PO PLA */
    new codepair(0x0dbc, 0x0e1c), /*               Thai_phophung HAI CHARACTER PHO PHUNG */
    new codepair(0x0dbd, 0x0e1d), /*                   Thai_fofa HAI CHARACTER FO FA */
    new codepair(0x0dbe, 0x0e1e), /*                Thai_phophan HAI CHARACTER PHO PHAN */
    new codepair(0x0dbf, 0x0e1f), /*                  Thai_fofan HAI CHARACTER FO FAN */
    new codepair(0x0dc0, 0x0e20), /*             Thai_phosamphao HAI CHARACTER PHO SAMPHAO */
    new codepair(0x0dc1, 0x0e21), /*                   Thai_moma HAI CHARACTER MO MA */
    new codepair(0x0dc2, 0x0e22), /*                  Thai_yoyak HAI CHARACTER YO YAK */
    new codepair(0x0dc3, 0x0e23), /*                  Thai_rorua HAI CHARACTER RO RUA */
    new codepair(0x0dc4, 0x0e24), /*                     Thai_ru HAI CHARACTER RU */
    new codepair(0x0dc5, 0x0e25), /*                 Thai_loling HAI CHARACTER LO LING */
    new codepair(0x0dc6, 0x0e26), /*                     Thai_lu HAI CHARACTER LU */
    new codepair(0x0dc7, 0x0e27), /*                 Thai_wowaen HAI CHARACTER WO WAEN */
    new codepair(0x0dc8, 0x0e28), /*                 Thai_sosala HAI CHARACTER SO SALA */
    new codepair(0x0dc9, 0x0e29), /*                 Thai_sorusi HAI CHARACTER SO RUSI */
    new codepair(0x0dca, 0x0e2a), /*                  Thai_sosua HAI CHARACTER SO SUA */
    new codepair(0x0dcb, 0x0e2b), /*                  Thai_hohip HAI CHARACTER HO HIP */
    new codepair(0x0dcc, 0x0e2c), /*                Thai_lochula HAI CHARACTER LO CHULA */
    new codepair(0x0dcd, 0x0e2d), /*                   Thai_oang HAI CHARACTER O ANG */
    new codepair(0x0dce, 0x0e2e), /*               Thai_honokhuk HAI CHARACTER HO NOKHUK */
    new codepair(0x0dcf, 0x0e2f), /*              Thai_paiyannoi HAI CHARACTER PAIYANNOI */
    new codepair(0x0dd0, 0x0e30), /*                  Thai_saraa HAI CHARACTER SARA A */
    new codepair(0x0dd1, 0x0e31), /*             Thai_maihanakat HAI CHARACTER MAI HAN-AKAT */
    new codepair(0x0dd2, 0x0e32), /*                 Thai_saraaa HAI CHARACTER SARA AA */
    new codepair(0x0dd3, 0x0e33), /*                 Thai_saraam HAI CHARACTER SARA AM */
    new codepair(0x0dd4, 0x0e34), /*                  Thai_sarai HAI CHARACTER SARA I */
    new codepair(0x0dd5, 0x0e35), /*                 Thai_saraii HAI CHARACTER SARA II */
    new codepair(0x0dd6, 0x0e36), /*                 Thai_saraue HAI CHARACTER SARA UE */
    new codepair(0x0dd7, 0x0e37), /*                Thai_sarauee HAI CHARACTER SARA UEE */
    new codepair(0x0dd8, 0x0e38), /*                  Thai_sarau HAI CHARACTER SARA U */
    new codepair(0x0dd9, 0x0e39), /*                 Thai_sarauu HAI CHARACTER SARA UU */
    new codepair(0x0dda, 0x0e3a), /*                Thai_phinthu HAI CHARACTER PHINTHU */
  /*  0x0dde                    Thai_maihanakat_maitho ? ??? */
    new codepair(0x0ddf, 0x0e3f), /*                   Thai_baht HAI CURRENCY SYMBOL BAHT */
    new codepair(0x0de0, 0x0e40), /*                  Thai_sarae HAI CHARACTER SARA E */
    new codepair(0x0de1, 0x0e41), /*                 Thai_saraae HAI CHARACTER SARA AE */
    new codepair(0x0de2, 0x0e42), /*                  Thai_sarao HAI CHARACTER SARA O */
    new codepair(0x0de3, 0x0e43), /*          Thai_saraaimaimuan HAI CHARACTER SARA AI MAIMUAN */
    new codepair(0x0de4, 0x0e44), /*         Thai_saraaimaimalai HAI CHARACTER SARA AI MAIMALAI */
    new codepair(0x0de5, 0x0e45), /*            Thai_lakkhangyao HAI CHARACTER LAKKHANGYAO */
    new codepair(0x0de6, 0x0e46), /*               Thai_maiyamok HAI CHARACTER MAIYAMOK */
    new codepair(0x0de7, 0x0e47), /*              Thai_maitaikhu HAI CHARACTER MAITAIKHU */
    new codepair(0x0de8, 0x0e48), /*                  Thai_maiek HAI CHARACTER MAI EK */
    new codepair(0x0de9, 0x0e49), /*                 Thai_maitho HAI CHARACTER MAI THO */
    new codepair(0x0dea, 0x0e4a), /*                 Thai_maitri HAI CHARACTER MAI TRI */
    new codepair(0x0deb, 0x0e4b), /*            Thai_maichattawa HAI CHARACTER MAI CHATTAWA */
    new codepair(0x0dec, 0x0e4c), /*            Thai_thanthakhat HAI CHARACTER THANTHAKHAT */
    new codepair(0x0ded, 0x0e4d), /*               Thai_nikhahit HAI CHARACTER NIKHAHIT */
    new codepair(0x0df0, 0x0e50), /*                 Thai_leksun HAI DIGIT ZERO */
    new codepair(0x0df1, 0x0e51), /*                Thai_leknung HAI DIGIT ONE */
    new codepair(0x0df2, 0x0e52), /*                Thai_leksong HAI DIGIT TWO */
    new codepair(0x0df3, 0x0e53), /*                 Thai_leksam HAI DIGIT THREE */
    new codepair(0x0df4, 0x0e54), /*                  Thai_leksi HAI DIGIT FOUR */
    new codepair(0x0df5, 0x0e55), /*                  Thai_lekha HAI DIGIT FIVE */
    new codepair(0x0df6, 0x0e56), /*                 Thai_lekhok HAI DIGIT SIX */
    new codepair(0x0df7, 0x0e57), /*                Thai_lekchet HAI DIGIT SEVEN */
    new codepair(0x0df8, 0x0e58), /*                Thai_lekpaet HAI DIGIT EIGHT */
    new codepair(0x0df9, 0x0e59), /*                 Thai_lekkao HAI DIGIT NINE */
    new codepair(0x0ea1, 0x3131), /*               Hangul_Kiyeog ANGUL LETTER KIYEOK */
    new codepair(0x0ea2, 0x3132), /*          Hangul_SsangKiyeog ANGUL LETTER SSANGKIYEOK */
    new codepair(0x0ea3, 0x3133), /*           Hangul_KiyeogSios ANGUL LETTER KIYEOK-SIOS */
    new codepair(0x0ea4, 0x3134), /*                Hangul_Nieun ANGUL LETTER NIEUN */
    new codepair(0x0ea5, 0x3135), /*           Hangul_NieunJieuj ANGUL LETTER NIEUN-CIEUC */
    new codepair(0x0ea6, 0x3136), /*           Hangul_NieunHieuh ANGUL LETTER NIEUN-HIEUH */
    new codepair(0x0ea7, 0x3137), /*               Hangul_Dikeud ANGUL LETTER TIKEUT */
    new codepair(0x0ea8, 0x3138), /*          Hangul_SsangDikeud ANGUL LETTER SSANGTIKEUT */
    new codepair(0x0ea9, 0x3139), /*                Hangul_Rieul ANGUL LETTER RIEUL */
    new codepair(0x0eaa, 0x313a), /*          Hangul_RieulKiyeog ANGUL LETTER RIEUL-KIYEOK */
    new codepair(0x0eab, 0x313b), /*           Hangul_RieulMieum ANGUL LETTER RIEUL-MIEUM */
    new codepair(0x0eac, 0x313c), /*           Hangul_RieulPieub ANGUL LETTER RIEUL-PIEUP */
    new codepair(0x0ead, 0x313d), /*            Hangul_RieulSios ANGUL LETTER RIEUL-SIOS */
    new codepair(0x0eae, 0x313e), /*           Hangul_RieulTieut ANGUL LETTER RIEUL-THIEUTH */
    new codepair(0x0eaf, 0x313f), /*          Hangul_RieulPhieuf ANGUL LETTER RIEUL-PHIEUPH */
    new codepair(0x0eb0, 0x3140), /*           Hangul_RieulHieuh ANGUL LETTER RIEUL-HIEUH */
    new codepair(0x0eb1, 0x3141), /*                Hangul_Mieum ANGUL LETTER MIEUM */
    new codepair(0x0eb2, 0x3142), /*                Hangul_Pieub ANGUL LETTER PIEUP */
    new codepair(0x0eb3, 0x3143), /*           Hangul_SsangPieub ANGUL LETTER SSANGPIEUP */
    new codepair(0x0eb4, 0x3144), /*            Hangul_PieubSios ANGUL LETTER PIEUP-SIOS */
    new codepair(0x0eb5, 0x3145), /*                 Hangul_Sios ANGUL LETTER SIOS */
    new codepair(0x0eb6, 0x3146), /*            Hangul_SsangSios ANGUL LETTER SSANGSIOS */
    new codepair(0x0eb7, 0x3147), /*                Hangul_Ieung ANGUL LETTER IEUNG */
    new codepair(0x0eb8, 0x3148), /*                Hangul_Jieuj ANGUL LETTER CIEUC */
    new codepair(0x0eb9, 0x3149), /*           Hangul_SsangJieuj ANGUL LETTER SSANGCIEUC */
    new codepair(0x0eba, 0x314a), /*                Hangul_Cieuc ANGUL LETTER CHIEUCH */
    new codepair(0x0ebb, 0x314b), /*               Hangul_Khieuq ANGUL LETTER KHIEUKH */
    new codepair(0x0ebc, 0x314c), /*                Hangul_Tieut ANGUL LETTER THIEUTH */
    new codepair(0x0ebd, 0x314d), /*               Hangul_Phieuf ANGUL LETTER PHIEUPH */
    new codepair(0x0ebe, 0x314e), /*                Hangul_Hieuh ANGUL LETTER HIEUH */
    new codepair(0x0ebf, 0x314f), /*                    Hangul_A ANGUL LETTER A */
    new codepair(0x0ec0, 0x3150), /*                   Hangul_AE ANGUL LETTER AE */
    new codepair(0x0ec1, 0x3151), /*                   Hangul_YA ANGUL LETTER YA */
    new codepair(0x0ec2, 0x3152), /*                  Hangul_YAE ANGUL LETTER YAE */
    new codepair(0x0ec3, 0x3153), /*                   Hangul_EO ANGUL LETTER EO */
    new codepair(0x0ec4, 0x3154), /*                    Hangul_E ANGUL LETTER E */
    new codepair(0x0ec5, 0x3155), /*                  Hangul_YEO ANGUL LETTER YEO */
    new codepair(0x0ec6, 0x3156), /*                   Hangul_YE ANGUL LETTER YE */
    new codepair(0x0ec7, 0x3157), /*                    Hangul_O ANGUL LETTER O */
    new codepair(0x0ec8, 0x3158), /*                   Hangul_WA ANGUL LETTER WA */
    new codepair(0x0ec9, 0x3159), /*                  Hangul_WAE ANGUL LETTER WAE */
    new codepair(0x0eca, 0x315a), /*                   Hangul_OE ANGUL LETTER OE */
    new codepair(0x0ecb, 0x315b), /*                   Hangul_YO ANGUL LETTER YO */
    new codepair(0x0ecc, 0x315c), /*                    Hangul_U ANGUL LETTER U */
    new codepair(0x0ecd, 0x315d), /*                  Hangul_WEO ANGUL LETTER WEO */
    new codepair(0x0ece, 0x315e), /*                   Hangul_WE ANGUL LETTER WE */
    new codepair(0x0ecf, 0x315f), /*                   Hangul_WI ANGUL LETTER WI */
    new codepair(0x0ed0, 0x3160), /*                   Hangul_YU ANGUL LETTER YU */
    new codepair(0x0ed1, 0x3161), /*                   Hangul_EU ANGUL LETTER EU */
    new codepair(0x0ed2, 0x3162), /*                   Hangul_YI ANGUL LETTER YI */
    new codepair(0x0ed3, 0x3163), /*                    Hangul_I ANGUL LETTER I */
    new codepair(0x0ed4, 0x11a8), /*             Hangul_J_Kiyeog ANGUL JONGSEONG KIYEOK */
    new codepair(0x0ed5, 0x11a9), /*        Hangul_J_SsangKiyeog ANGUL JONGSEONG SSANGKIYEOK */
    new codepair(0x0ed6, 0x11aa), /*         Hangul_J_KiyeogSios ANGUL JONGSEONG KIYEOK-SIOS */
    new codepair(0x0ed7, 0x11ab), /*              Hangul_J_Nieun ANGUL JONGSEONG NIEUN */
    new codepair(0x0ed8, 0x11ac), /*         Hangul_J_NieunJieuj ANGUL JONGSEONG NIEUN-CIEUC */
    new codepair(0x0ed9, 0x11ad), /*         Hangul_J_NieunHieuh ANGUL JONGSEONG NIEUN-HIEUH */
    new codepair(0x0eda, 0x11ae), /*             Hangul_J_Dikeud ANGUL JONGSEONG TIKEUT */
    new codepair(0x0edb, 0x11af), /*              Hangul_J_Rieul ANGUL JONGSEONG RIEUL */
    new codepair(0x0edc, 0x11b0), /*        Hangul_J_RieulKiyeog ANGUL JONGSEONG RIEUL-KIYEOK */
    new codepair(0x0edd, 0x11b1), /*         Hangul_J_RieulMieum ANGUL JONGSEONG RIEUL-MIEUM */
    new codepair(0x0ede, 0x11b2), /*         Hangul_J_RieulPieub ANGUL JONGSEONG RIEUL-PIEUP */
    new codepair(0x0edf, 0x11b3), /*          Hangul_J_RieulSios ANGUL JONGSEONG RIEUL-SIOS */
    new codepair(0x0ee0, 0x11b4), /*         Hangul_J_RieulTieut ANGUL JONGSEONG RIEUL-THIEUTH */
    new codepair(0x0ee1, 0x11b5), /*        Hangul_J_RieulPhieuf ANGUL JONGSEONG RIEUL-PHIEUPH */
    new codepair(0x0ee2, 0x11b6), /*         Hangul_J_RieulHieuh ANGUL JONGSEONG RIEUL-HIEUH */
    new codepair(0x0ee3, 0x11b7), /*              Hangul_J_Mieum ANGUL JONGSEONG MIEUM */
    new codepair(0x0ee4, 0x11b8), /*              Hangul_J_Pieub ANGUL JONGSEONG PIEUP */
    new codepair(0x0ee5, 0x11b9), /*          Hangul_J_PieubSios ANGUL JONGSEONG PIEUP-SIOS */
    new codepair(0x0ee6, 0x11ba), /*               Hangul_J_Sios ANGUL JONGSEONG SIOS */
    new codepair(0x0ee7, 0x11bb), /*          Hangul_J_SsangSios ANGUL JONGSEONG SSANGSIOS */
    new codepair(0x0ee8, 0x11bc), /*              Hangul_J_Ieung ANGUL JONGSEONG IEUNG */
    new codepair(0x0ee9, 0x11bd), /*              Hangul_J_Jieuj ANGUL JONGSEONG CIEUC */
    new codepair(0x0eea, 0x11be), /*              Hangul_J_Cieuc ANGUL JONGSEONG CHIEUCH */
    new codepair(0x0eeb, 0x11bf), /*             Hangul_J_Khieuq ANGUL JONGSEONG KHIEUKH */
    new codepair(0x0eec, 0x11c0), /*              Hangul_J_Tieut ANGUL JONGSEONG THIEUTH */
    new codepair(0x0eed, 0x11c1), /*             Hangul_J_Phieuf ANGUL JONGSEONG PHIEUPH */
    new codepair(0x0eee, 0x11c2), /*              Hangul_J_Hieuh ANGUL JONGSEONG HIEUH */
    new codepair(0x0eef, 0x316d), /*     Hangul_RieulYeorinHieuh ANGUL LETTER RIEUL-YEORINHIEUH */
    new codepair(0x0ef0, 0x3171), /*    Hangul_SunkyeongeumMieum ANGUL LETTER KAPYEOUNMIEUM */
    new codepair(0x0ef1, 0x3178), /*    Hangul_SunkyeongeumPieub ANGUL LETTER KAPYEOUNPIEUP */
    new codepair(0x0ef2, 0x317f), /*              Hangul_PanSios ANGUL LETTER PANSIOS */
    new codepair(0x0ef3, 0x3181), /*    Hangul_KkogjiDalrinIeung ANGUL LETTER YESIEUNG */
    new codepair(0x0ef4, 0x3184), /*   Hangul_SunkyeongeumPhieuf ANGUL LETTER KAPYEOUNPHIEUPH */
    new codepair(0x0ef5, 0x3186), /*          Hangul_YeorinHieuh ANGUL LETTER YEORINHIEUH */
    new codepair(0x0ef6, 0x318d), /*                Hangul_AraeA ANGUL LETTER ARAEA */
    new codepair(0x0ef7, 0x318e), /*               Hangul_AraeAE ANGUL LETTER ARAEAE */
    new codepair(0x0ef8, 0x11eb), /*            Hangul_J_PanSios ANGUL JONGSEONG PANSIOS */
    new codepair(0x0ef9, 0x11f0), /*  Hangul_J_KkogjiDalrinIeung ANGUL JONGSEONG YESIEUNG */
    new codepair(0x0efa, 0x11f9), /*        Hangul_J_YeorinHieuh ANGUL JONGSEONG YEORINHIEUH */
    new codepair(0x0eff, 0x20a9), /*                  Korean_Won ON SIGN */
    new codepair(0x13bc, 0x0152), /*                          OE TIN CAPITAL LIGATURE OE */
    new codepair(0x13bd, 0x0153), /*                          oe TIN SMALL LIGATURE OE */
    new codepair(0x13be, 0x0178), /*                  Ydiaeresis TIN CAPITAL LETTER Y WITH DIAERESIS */
    new codepair(0x20ac, 0x20ac), /*                    EuroSign URO SIGN */
    new codepair(0xfe50, 0x0300), /*                               COMBINING GRAVE ACCENT */
    new codepair(0xfe51, 0x0301), /*                               COMBINING ACUTE ACCENT */
    new codepair(0xfe52, 0x0302), /*                               COMBINING CIRCUMFLEX ACCENT */
    new codepair(0xfe53, 0x0303), /*                               COMBINING TILDE */
    new codepair(0xfe54, 0x0304), /*                               COMBINING MACRON */
    new codepair(0xfe55, 0x0306), /*                               COMBINING BREVE */
    new codepair(0xfe56, 0x0307), /*                               COMBINING DOT ABOVE */
    new codepair(0xfe57, 0x0308), /*                               COMBINING DIAERESIS */
    new codepair(0xfe58, 0x030a), /*                               COMBINING RING ABOVE */
    new codepair(0xfe59, 0x030b), /*                               COMBINING DOUBLE ACUTE ACCENT */
    new codepair(0xfe5a, 0x030c), /*                               COMBINING CARON */
    new codepair(0xfe5b, 0x0327), /*                               COMBINING CEDILLA */
    new codepair(0xfe5c, 0x0328), /*                               COMBINING OGONEK */
    new codepair(0xfe5d, 0x1da5), /*                               MODIFIER LETTER SMALL IOTA */
    new codepair(0xfe5e, 0x3099), /*                               COMBINING KATAKANA-HIRAGANA VOICED SOUND MARK */
    new codepair(0xfe5f, 0x309a), /*                               COMBINING KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK */
    new codepair(0xfe60, 0x0323), /*                               COMBINING DOT BELOW */
  };

  public static int ucs2keysym(int ucs) {
    int cur = 0;
    int max = keysymtab.length - 1;

    /* first check for Latin-1 characters (1:1 mapping) */
    if ((ucs >= 0x0020 && ucs <= 0x007e) ||
        (ucs >= 0x00a0 && ucs <= 0x00ff))
      return ucs;

    /* linear search in table */
    while (cur <= max) {
      if (keysymtab[cur].ucs == ucs)
        return keysymtab[cur].keysym;
      cur++;
    }

    /* use the directly encoded 24-bit UCS character */
    if ((ucs & 0xff000000) == 0)
      return ucs | 0x01000000;

    /* no matching Unicode value found */
    return 0xffffff;
  }
}
