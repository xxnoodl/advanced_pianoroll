#pragma once
#include <JuceHeader.h>

// Original vector glyphs in a 24-unit coordinate system; no bitmap or icon-font scaling.
inline void drawRollIcon(juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& name, bool active) {
    juce::Graphics::ScopedSaveState saved(g);
    const float size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    g.addTransform(juce::AffineTransform::scale(size / 24.f).translated(bounds.getCentreX() - size / 2, bounds.getCentreY() - size / 2));
    juce::Path p;
    auto line = [&](float x, float y, float xx, float yy) { p.startNewSubPath(x,y); p.lineTo(xx,yy); };
    if (name == "play") { if (active) p.addRectangle(6,6,12,12); else p.addTriangle(7,4,7,20,20,12); g.fillPath(p); return; }
    if (name == "host") { p.addRoundedRectangle(2,7,12,7,3); p.addRoundedRectangle(10,10,12,7,3); }
    else if (name == "monitor") { p.startNewSubPath(3,9); p.lineTo(7,9); p.lineTo(12,5); p.lineTo(12,19); p.lineTo(7,15); p.lineTo(3,15); p.closeSubPath(); line(16,8,19,11); line(19,11,19,13); line(19,13,16,16); }
    else if (name == "undo" || name == "redo") { p.startNewSubPath(5,9); p.cubicTo(21,2,24,23,9,19); line(5,9,11,9); line(5,9,6,3); if (name == "redo") p.applyTransform(juce::AffineTransform::scale(-1,1).translated(24,0)); }
    else if (name == "stamp") { p.addRoundedRectangle(8,3,8,9,3); line(8,12,5,17); line(16,12,19,17); line(5,17,19,17); line(4,21,20,21); }
    else if (name == "processing") { line(4,20,17,7); p.startNewSubPath(13,4); p.lineTo(13,9); p.lineTo(18,11); p.lineTo(22,7); p.lineTo(18,8); p.lineTo(16,6); p.lineTo(17,2); p.closeSubPath(); }
    else if (name == "fit") { line(3,9,3,3); line(3,3,9,3); line(15,3,21,3); line(21,3,21,9); line(21,15,21,21); line(21,21,15,21); line(9,21,3,21); line(3,21,3,15); }
    else if (name == "import" || name == "export") { line(3,16,3,21); line(3,21,21,21); line(21,21,21,16); line(12,3,12,16); line(7,11,12,16); line(12,16,17,11); if (name == "export") { p.clear(); line(3,16,3,21); line(3,21,21,21); line(21,21,21,16); line(12,16,12,3); line(7,8,12,3); line(12,3,17,8); } }
    else if (name == "routing") { line(3,12,11,12); line(11,12,11,5); line(11,5,21,5); line(11,12,11,19); line(11,19,21,19); line(18,2,21,5); line(21,5,18,8); line(18,16,21,19); line(21,19,18,22); }
    else if (name == "save") { p.addRectangle(4,3,16,18); p.addRectangle(8,3,8,6); p.addRectangle(8,14,8,7); }
    else if (name == "load") { p.startNewSubPath(2,20); p.lineTo(2,5); p.lineTo(9,5); p.lineTo(12,8); p.lineTo(21,8); p.lineTo(21,11); p.startNewSubPath(2,20); p.lineTo(6,11); p.lineTo(23,11); p.lineTo(19,20); p.closeSubPath(); }
    else if (name == "typing") { p.addRoundedRectangle(2,4,20,16,2); for (int x : {7,12,17}) line(static_cast<float>(x),5,static_cast<float>(x),19); for (int x : {6,11,16}) p.addRectangle(static_cast<float>(x),5,2,7); }
    else if (name == "draw") { p.startNewSubPath(4,20); p.lineTo(5,14); p.lineTo(17,2); p.lineTo(22,7); p.lineTo(10,19); p.closeSubPath(); line(14,5,19,10); }
    else if (name == "paint") { p.addRoundedRectangle(4,3,16,7,2); line(18,10,18,13); line(18,13,12,13); line(12,13,12,21); }
    else if (name == "select") { for (int x : {3,10,17}) { line(static_cast<float>(x),3,static_cast<float>(x+3),3); line(static_cast<float>(x),21,static_cast<float>(x+3),21); } for (int y : {3,10,17}) { line(3,static_cast<float>(y),3,static_cast<float>(y+3)); line(21,static_cast<float>(y),21,static_cast<float>(y+3)); } }
    else if (name == "mute") { p.addEllipse(3,3,18,18); line(5,19,19,5); }
    else if (name == "slice") { p.addEllipse(3,15,6,6); p.addEllipse(15,15,6,6); line(7,16,18,3); line(17,16,6,3); }
    else { p.addEllipse(3,3,18,18); p.startNewSubPath(9,8); p.cubicTo(9,3,20,7,13,12); p.lineTo(12,14); p.addEllipse(11.3f,17,1.4f,1.4f); }
    g.strokePath(p,juce::PathStrokeType(1.55f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
}
