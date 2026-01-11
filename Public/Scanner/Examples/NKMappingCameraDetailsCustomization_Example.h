// Example: Custom Details Panel for NKMappingCamera
// This would go in a new Editor module (requires creating an Editor plugin/module)

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

/**
 * Custom details panel for ANKMappingCamera
 * Provides a cleaner, more organized interface for scanner properties
 */
class FNKMappingCameraDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	/** Build the Scanner Settings category */
	void BuildScannerSettingsCategory(IDetailLayoutBuilder& DetailBuilder);
	
	/** Build the Discovery Settings category */
	void BuildDiscoverySettingsCategory(IDetailLayoutBuilder& DetailBuilder);
	
	/** Build the Camera Controls category */
	void BuildCameraControlsCategory(IDetailLayoutBuilder& DetailBuilder);
};

// In the .cpp file:
/*
#include "NKMappingCameraDetails.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Scanner/NKMappingCamera.h"

TSharedRef<IDetailCustomization> FNKMappingCameraDetails::MakeInstance()
{
	return MakeShareable(new FNKMappingCameraDetails);
}

void FNKMappingCameraDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Hide default categories you don't want
	DetailBuilder.HideCategory("Rendering");
	DetailBuilder.HideCategory("Replication");
	DetailBuilder.HideCategory("Collision");
	DetailBuilder.HideCategory("Input");
	DetailBuilder.HideCategory("Actor");
	
	// Build custom categories
	BuildScannerSettingsCategory(DetailBuilder);
	BuildDiscoverySettingsCategory(DetailBuilder);
	BuildCameraControlsCategory(DetailBuilder);
}

void FNKMappingCameraDetails::BuildScannerSettingsCategory(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Scanner Configuration", 
		FText::FromString("Scanner Configuration"), ECategoryPriority::Important);
	
	// Add properties in custom order
	TSharedRef<IPropertyHandle> TargetActorProperty = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(ANKMappingCamera, TargetActor));
	Category.AddProperty(TargetActorProperty);
	
	// Add more properties...
}

// Register in your editor module:
void FYourEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(
		ANKMappingCamera::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FNKMappingCameraDetails::MakeInstance)
	);
}
*/
