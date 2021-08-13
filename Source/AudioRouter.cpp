/**
    bespoke synth, a software modular synthesizer
    Copyright (C) 2021 Ryan Challinor (contact: awwbees@gmail.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/
//
//  AudioRouter.cpp
//  modularSynth
//
//  Created by Ryan Challinor on 4/7/13.
//
//

#include "AudioRouter.h"
#include "ModularSynth.h"
#include "Profiler.h"
#include "PatchCableSource.h"

AudioRouter::AudioRouter()
: IAudioProcessor(gBufferSize)
, mRouteIndex(0)
, mRouteSelector(nullptr)
, mBlankVizBuffer(VIZ_BUFFER_SECONDS*gSampleRate)
{
}

void AudioRouter::CreateUIControls()
{
   IDrawableModule::CreateUIControls();
   mRouteSelector = new RadioButton(this,"route",5,3,&mRouteIndex);
   
   GetPatchCableSource()->SetEnabled(false);
}

AudioRouter::~AudioRouter()
{
}

void AudioRouter::Process(double time)
{
   PROFILER(AudioRouter);

   SyncBuffers();
   mBlankVizBuffer.SetNumChannels(GetBuffer()->NumActiveChannels());
   
   for (size_t i=0; i<mDestinationCables.size(); ++i)
   {
      if ((int)i == mRouteIndex)
         mDestinationCables[i]->SetOverrideVizBuffer(nullptr);
      else
         mDestinationCables[i]->SetOverrideVizBuffer(&mBlankVizBuffer);
   }
   
   IAudioReceiver* target = GetTarget(mRouteIndex+1);
   if (target == nullptr)
      return;

   for (int ch=0; ch<GetBuffer()->NumActiveChannels(); ++ch)
   {
      Add(target->GetBuffer()->GetChannel(ch), GetBuffer()->GetChannel(ch), GetBuffer()->BufferSize());
      GetVizBuffer()->WriteChunk(GetBuffer()->GetChannel(ch),GetBuffer()->BufferSize(), ch);
   }

   GetBuffer()->Reset();
}

void AudioRouter::DrawModule()
{
   if (Minimized() || IsVisible() == false)
      return;

   mRouteSelector->Draw();
   
   for (int i=0; i<(int)mDestinationCables.size(); ++i)
   {
      ofVec2f pos = mRouteSelector->GetOptionPosition(i) - mRouteSelector->GetPosition();
      mDestinationCables[i]->SetManualPosition(pos.x + 10, pos.y + 4);
   }
}

void AudioRouter::PostRepatch(PatchCableSource* cableSource, bool fromUserClick)
{
   IAudioSource::PostRepatch(cableSource, fromUserClick);
   
   for (int i=0; i<(int)mDestinationCables.size(); ++i)
   {
      if (cableSource == mDestinationCables[i])
      {
         IClickable* target = cableSource->GetTarget();
         string name = target ? target->Name() : "                      ";
         mRouteSelector->SetLabel(name.c_str(), i);
      }
   }
}

void AudioRouter::GetModuleDimensions(float& width, float& height)
{
   float w,h;
   mRouteSelector->GetDimensions(w, h);
   width = 10+w;
   height = 8+h;
}

void AudioRouter::RadioButtonUpdated(RadioButton* radio, int oldVal)
{
   if (radio == mRouteSelector)
   {
   }
}

void AudioRouter::LoadLayout(const ofxJSONElement& moduleInfo)
{
   mModuleSaveData.LoadInt("num_items",moduleInfo,2,1,99,K(isTextField));
   
   SetUpFromSaveData();
}

void AudioRouter::SetUpFromSaveData()
{
   int numItems = mModuleSaveData.GetInt("num_items");
   int oldNumItems = (int)mDestinationCables.size();
   if (numItems > oldNumItems)
   {
      for (int i=oldNumItems; i<numItems; ++i)
      {
         mRouteSelector->AddLabel("                      ", i);
         auto* additionalCable = new PatchCableSource(this, kConnectionType_Audio);
         AddPatchCableSource(additionalCable);
         mDestinationCables.push_back(additionalCable);
      }
   }
   else if (numItems < oldNumItems)
   {
      for (int i=oldNumItems-1; i>=numItems; --i)
      {
         mRouteSelector->RemoveLabel(i);
         RemovePatchCableSource(mDestinationCables[i]);
      }
      mDestinationCables.resize(numItems);
   }
}

void AudioRouter::SaveLayout(ofxJSONElement& moduleInfo)
{
   IDrawableModule::SaveLayout(moduleInfo);
   
   moduleInfo["num_items"] = (int)mDestinationCables.size();
}

